/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_common.h"
#include "ack_core_communication.h"
#include "ack_core_directives.h"
#include "ack_core_document.h"
#include "ack_core_document_wrappers.h"
#include "ack_core_event_builder.h"
#include "ack_core_fixed_memory.h"
#include "ack_core_heaplet.h"
#include "ack_core_lifecycle.h"
#include "ack_core_processevent.h"
#include "ack_core_protobuf_utilities.h"
#include "ack_core_validation.h"
#include "ack_user_config.h"
#include "acp.pb.h"
#include "ack_logging.h"
#include "ack_user_platform.h"
#include "ack_user_module.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

// Event processing advances through a series of states to avoid deep call trees and
// excessive stack usage.
typedef enum ProcessEventState
{
    ProcessEventState_ReceiveNextEvent,
    ProcessEventState_DirectiveExtractBinaryDocument,
    ProcessEventState_DirectiveProcess,
    ProcessEventState_ReceivedBootedEvent,
    ProcessEventState_ModuleReqestingRebootEvent,
    ProcessEventState_ReceivedFactoryResetEvent,
#ifdef ACK_HOST_FIRMWARE_UPDATE
    ProcessEventState_HostFirmwareUpdateReceived,
#endif
#ifdef ACK_LOGGING
    ProcessEventState_LogLevelChangedEvent,
#endif
    ProcessEventState_Max
}
ProcessEventState_t;

static ACKError_t ProcessNextEvent_(void);
static ACKError_t ProcessEventState_ReceiveNextEvent_(void);
static ACKError_t ProcessEventState_DirectiveExtractBinaryDocument_(void);
static ACKError_t ProcessEventState_DirectiveProcess_(void);
static ACKError_t ProcessEventState_ReceivedBootedEvent_(void);
static ACKError_t ProcessEventState_ModuleRequestingRebootEvent_(void);
static ACKError_t ProcessEventState_ReceivedFactoryResetEvent_(void);
#ifdef ACK_LOGGING
static ACKError_t ProcessEventState_LogLevelChangedEvent_(void);
#endif

#ifdef ACK_HOST_FIRMWARE_UPDATE
static ACKError_t ProcessEventState_HostFirmwareUpdateReceived_(void);
#endif

static void FreeAcpResponse_(void);
static void FreeDirectiveDocument_(void);

typedef ACKError_t (*ProcessEventStateProcessor)(void);

// When assertions are turned on, after processing each state we check the heaplet to ensure that only
// the memory we expect to be allocated is actually allocated.
typedef struct StateContext
{
    ProcessEventStateProcessor m_processor;
}
StateContext_t;

#define STATE(n) { ProcessEventState_##n##_ }

static const StateContext_t sg_processEventStateProcessors[] =
{
    STATE(ReceiveNextEvent),
    STATE(DirectiveExtractBinaryDocument),
    STATE(DirectiveProcess),
    STATE(ReceivedBootedEvent),
    STATE(ModuleRequestingRebootEvent),
    STATE(ReceivedFactoryResetEvent)
#ifdef ACK_HOST_FIRMWARE_UPDATE
   ,STATE(HostFirmwareUpdateReceived)
#endif
#ifdef ACK_LOGGING
   ,STATE(LogLevelChangedEvent)
#endif
};

static ProcessEventState_t sg_state;

ACKInboundEventInfo_t g_inboundEventInfo = {{0}};

ACKError_t ProcessEventIfAvailable(void)
{
    ACKError_t retval = ACK_NO_ERROR;

    g_inboundEventInfo.Bits.ProcessedDirective = false;

    if (ACK_IsHostInterruptAsserted())
    {
        ACK_DEBUG_PRINT_I("HOSTINT asserted; processing inbound event.");
			
        g_inboundEventInfo.Bits.Processing = true;

        retval = ProcessNextEvent_();
    }

    g_inboundEventInfo.Bits.Processing = false;

    return retval;
}

static ACKError_t ProcessNextEvent_(void)
{
    ACKError_t retval = ACK_NO_ERROR;

    // Because of the way memory is managed, we can't process an inbound event if there's an outbound one
    // pending. In some cases, user code may legitimately call ACK_Complete* but the event never got
    // sent. To avoid the need for an explicit ACK_AbandonEvent, we just remove any pending event here.
#ifdef ACK_EVENT_BUILDER_DIAG
    if (EventBuilderIsEventBeingBuilt())
    {
        ACK_DEBUG_PRINT_W("Deleting unsent pending outgoing event.");
    }
#endif
    EventBuilderEnd();

    sg_state = ProcessEventState_ReceiveNextEvent;

    while ((ProcessEventState_Max != sg_state) && (ACK_NO_ERROR == retval))
    {
        retval = sg_processEventStateProcessors[sg_state].m_processor();
    }

#ifdef ACK_MEMORY_DIAG
    // Make sure we didn't leak memory. However an ACKUser_xxx callback may have started an
    // outbound event, so allow that. That includes at most MEM_OUTBOUND_BINARY_DOCUMENT.
    {
        HeapletTag_t tags[25];
        bool tagsOk;
        size_t i;

        // Tag count returned from HeapletGetBlocksInUse includes the terminating MEM_NONE.
        size_t tagCount = HeapletGetBlocksInUse(tags, ACK_COUNTOF(tags));

        tagsOk = (1 == tagCount) // must be MEM_NONE
                    || ((2 == tagCount) && (MEM_OUTBOUND_BINARY_DOCUMENT == tags[0]));

        if (!tagsOk)
        {
            if (tagCount > ACK_COUNTOF(tags))
            {
                tagCount = ACK_COUNTOF(tags);
            }

            for (i = 0; i < tagCount; ++i)
            {
                ACK_DEBUG_PRINT_I("At least block leaked: tag %u", tags[i]);
            }

            ACK_ASSERT(!"Processing incoming event leaked memory!");
        }
    }
#endif

    return retval;
}

// Processes the state wherein we send a request for the next event and read a response.
// At the end of this state, fixed acp_response memory contains the decoded acp_response struct,
// and MEM_RECEIVED_ENCODED_ACP_RESPONSE contains the original acp_response. The latter is kept around
// because the acp_response may have a binary payload that we still need to access later.
// The next state depends on what kind of event we received.
static ACKError_t ProcessEventState_ReceiveNextEvent_(void)
{
    ACKError_t retval;

    // This receives an encoded acp_response into MEM_RECEIVED_ENCODED_ACP_RESPONSE, and also decodes
    // *only the acp_response struct itself* into acp_response fixed mem. However acp_response may also
    // have an accompanying binary payload, which remains in MEM_RECEIVED_ENCODED_ACP_RESPONSE.
    // So we must keep both of those around for later use, i.e. we can't free the encoded one now.
    retval = ACKModule_GetResponse(&g_fixedAcpMemory.Response);
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("ACKModule_GetResponse failed, err %u.", retval);
        goto cleanup0;
    }

    // Advance to next state, which depends on the kind of event.
    switch (g_fixedAcpMemory.Response.which_type)
    {
    case acp_response_alexa_directive_received_obj_tag:
        ACK_DEBUG_PRINT_I("Received Alexa directive.");
        sg_state = ProcessEventState_DirectiveExtractBinaryDocument;
        break;

    case acp_response_ack_booted_obj_tag:
        ACK_DEBUG_PRINT_I("Received connectivity module booted event.");
        sg_state = ProcessEventState_ReceivedBootedEvent;
        break;

    case acp_response_reboot_requested_obj_tag:
        ACK_DEBUG_PRINT_I("Received event indicating ACK connectivity module is requesting to be rebooted.");
        sg_state = ProcessEventState_ModuleReqestingRebootEvent;
        break;

    case acp_response_factory_reset_requested_obj_tag:
        ACK_DEBUG_PRINT_I("Received event indicating that host should factory reset itself.");
        sg_state = ProcessEventState_ReceivedFactoryResetEvent;
        break;

#ifdef ACK_HOST_FIRMWARE_UPDATE
    case acp_response_host_fw_update_received_obj_tag:
        ACK_DEBUG_PRINT_I("Received host firmware update received event.");
        sg_state = ProcessEventState_HostFirmwareUpdateReceived;
        break;
#endif

#ifdef ACK_LOGGING
    case acp_response_log_level_changed_obj_tag:
        ACK_DEBUG_PRINT_I("Received log level changed event.");
        sg_state = ProcessEventState_LogLevelChangedEvent;
        break;
#endif

    case 0:
        // This can happen when
        // (a) the payload is omitted for some reason at the sending end (unlikely). The tag was sent as 0.
        // (b) we simply don't have handling for the tag we received. The tag became 0 when decoded.
        // We simply ignore these messages.
        ACK_DEBUG_PRINT_D("Ignored message with no payload.");
        sg_state = ProcessEventState_Max;
        break;

    default:
        // Unknown message. This happens when the payload is something we understand and could
        // potentially handle, but for some reason we didn't code.
        ACK_DEBUG_PRINT_W("Event of acp_response type %u not handled.", g_fixedAcpMemory.Response.which_type);
        sg_state = ProcessEventState_Max;
        break;
    }

    retval = ACK_NO_ERROR;

cleanup0:
    if ((ACK_NO_ERROR != retval) || (ProcessEventState_Max == sg_state))
    {
        FreeAcpResponse_();
    }

    return retval;
}

// Processes the state wherein we find the directive payload, which will be nested inside the
// encoded event data we originally received.
// At the end of this state,
// * MEM_RECEIVED_DIRECTIVE_INFO contains envelope info from the V3AVS_AvsDirective in the acp_response
// * MEM_RECEIVED_BINARY_DOCUMENT contains the extracted directive payload binary.
// and
// * MEM_RECEIVED_ENCODED_ACP_RESPONSE has been freed
static ACKError_t ProcessEventState_DirectiveExtractBinaryDocument_(void)
{
    acp_event_alexa_directive_received* pDirective;
    ACKError_t retval;
    ACKDirectiveParams_t* pDirectiveDocumentInfo;
    void* pDirectiveBinary;
    size_t binaryPayloadLength;
    const uint8_t* pBinaryPayload;

    pDirective = &g_fixedAcpMemory.Response.type.alexa_directive_received_obj;

    retval = ACKModule_GetEncodedDirective(pDirective, &pBinaryPayload, &binaryPayloadLength);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Allocate space for the ACKDirectiveParams_t struct, and the DOM Binary Form document.
    // Make sure that the order of the allocations here doesn't cause the first block allocated to move
    // as a result of allocating the second block.
    ACK_STATIC_ASSERT(MEM_RECEIVED_DIRECTIVE_INFO < MEM_RECEIVED_BINARY_DOCUMENT);
    pDirectiveDocumentInfo = HeapletSetSize(MEM_RECEIVED_DIRECTIVE_INFO, sizeof(ACKDirectiveParams_t));
    pDirectiveBinary = HeapletSetSize(MEM_RECEIVED_BINARY_DOCUMENT, binaryPayloadLength);

    if ((NULL == pDirectiveDocumentInfo) || (NULL == pDirectiveBinary))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    // Copy envelope info and the binary payload out of fixed acp_response memory and
    // MEM_RECEIVED_ENCODED_ACP_RESPONSE, respectively.
    // Note that we store the envelope namespace in here for now. It will get overwritten in
    // more specific processing such as ProcessDomBinaryFormDirective.
    pDirectiveDocumentInfo->CorrelationId = pDirective->directive.correlation_id;
    pDirectiveDocumentInfo->Namespace = (ack_namespace)pDirective->directive.v3_namespace;
    pDirectiveDocumentInfo->Instance = pDirective->directive.instance;

    memcpy(pDirectiveBinary, pBinaryPayload, binaryPayloadLength);

    ACK_DEBUG_PRINT_I(
        "Received directive; correlation id %"PRId32", namespace %u",
        pDirective->directive.correlation_id,
        pDirective->directive.v3_namespace);

    // Advance to next state.
    sg_state = ProcessEventState_DirectiveProcess;
    retval = ACK_NO_ERROR;

cleanup0:
    // Both fixed acp_response memory and MEM_RECEIVED_ENCODED_ACP_RESPONSE have been needed up to
    // this point because fixed acp_response memory is the acp_response struct *only* -- it does not
    // include the binary payload. MEM_RECEIVED_ENCODED_ACP_RESPONSE is the original response that
    // *does* include the binary payload.
    // Now that we've extracted the directive's envelope info and the binary payload, we no longer need
    // either of these.
    FreeAcpResponse_();

    if (ACK_NO_ERROR != retval)
    {
        FreeDirectiveDocument_();
    }

    return retval;
}

// Processes the state wherein a directive-specific handler is called to actually perform the processing
// associated with a directive.
// At the end of this state,
//  - MEM_RECEIVED_DIRECTIVE_INFO and MEM_RECEIVED_BINARY_DOCUMENT have been freed.
// The next state is completion.
static ACKError_t ProcessEventState_DirectiveProcess_(void)
{
    ACKDirectiveParams_t* pDirectiveDocumentInfo;
    ACKError_t retval;

    // The only kind of messages we know how to process are DOM Binary Form.
    pDirectiveDocumentInfo = (ACKDirectiveParams_t*)HeapletAccess(MEM_RECEIVED_DIRECTIVE_INFO);
    ACK_ASSERT(NULL != pDirectiveDocumentInfo);
    if (ACK_DOM_BINARY_FORM_ENVELOPE_NAMESPACE != (V3AVS_Namespace)pDirectiveDocumentInfo->Namespace)
    {
        ACK_DEBUG_PRINT_E(
            "Directive envelope namespace %u does not indicate a DOM Binary Form message.",
            pDirectiveDocumentInfo->Namespace);
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    retval = ProcessDomBinaryFormDirective(MEM_RECEIVED_BINARY_DOCUMENT, MEM_RECEIVED_DIRECTIVE_INFO);
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Directive-specific processing failed, err %u.", retval);
        goto cleanup0;
    }

    g_inboundEventInfo.Bits.ProcessedDirective = true;

    // Advance to completion state.
    sg_state = ProcessEventState_Max;
    retval = ACK_NO_ERROR;

cleanup0:
    // We no longer need the directive document or the directive payload.
    FreeDirectiveDocument_();

    return retval;
}

// Processes the state wherein we've received a booted event.
static ACKError_t ProcessEventState_ReceivedBootedEvent_(void)
{
    // The original acp_response and decoded form are no longer needed.
    FreeAcpResponse_();

    // Advance the lifecycle.
    LifecycleModuleBooted();

    // Done.
    sg_state = ProcessEventState_Max;
    return ACK_NO_ERROR;
}

// Processes the state wherein we've received a request from the connectivity module to be rebooted.
static ACKError_t ProcessEventState_ModuleRequestingRebootEvent_(void)
{
    bool force;

    force = g_fixedAcpMemory.Response.type.reboot_requested_obj.force;

    // The original acp_response and decoded form are now no longer needed.
    FreeAcpResponse_();

    // Advance the lifecycle.
    LifecycleModuleRequestingReboot(force);

    // Done.
    sg_state = ProcessEventState_Max;
    return ACK_NO_ERROR;
}

// Processes the state wherein we've received a factory reset event.
static ACKError_t ProcessEventState_ReceivedFactoryResetEvent_(void)
{
    // The original acp_response and decoded form are no longer needed.
    FreeAcpResponse_();

    // Advance the lifecycle.
    LifecycleFactoryResetRequested();

    // Done.
    sg_state = ProcessEventState_Max;
    return ACK_NO_ERROR;
}

#ifdef ACK_HOST_FIRMWARE_UPDATE

// Processes the state wherein we've received an update firmware event.
static ACKError_t ProcessEventState_HostFirmwareUpdateReceived_(void)
{
    acp_event_host_fw_update_received* pUpdateInfo;
    ACKError_t retval;

    ACK_DEBUG_PRINT_D("Calling host FW update received event handler.");

    pUpdateInfo = &g_fixedAcpMemory.Response.type.host_fw_update_received_obj;

    // Data we received can't be completely trusted. The two strings are of particular interest;
    // make sure they are terminated.
    ACK_STATIC_ASSERT(sizeof(pUpdateInfo->version) > 0);
    pUpdateInfo->version[sizeof(pUpdateInfo->version) - 1] = '\0';

    ACK_STATIC_ASSERT(sizeof(pUpdateInfo->signature) > 0);
    pUpdateInfo->signature[sizeof(pUpdateInfo->signature) - 1] = '\0';

    // Advance the lifecycle.
    LifecycleHostFirmwareUpdateReceived(
        pUpdateInfo->version,
        pUpdateInfo->signature,
        pUpdateInfo->size,
        pUpdateInfo->force);

    // Done.
    sg_state = ProcessEventState_Max;
    retval = ACK_NO_ERROR;

// There is no error path, so no need for this label.
//cleanup0:
    FreeAcpResponse_();
    return retval;
}

#endif // def ACK_HOST_FIRMWARE_UPDATE

#ifdef ACK_LOGGING

// Processes the state wherein we've received the log level changed event.
static ACKError_t ProcessEventState_LogLevelChangedEvent_(void)
{
    acp_event_log_level_changed* pMessage;
    ACKError_t retval;

    pMessage = &g_fixedAcpMemory.Response.type.log_level_changed_obj;
    ACK_DEBUG_PRINT_I("Log level change event received; new level is %u.", pMessage->log_level);

    // Simply adjust the global log level variable.
    ACK_LogLevel = pMessage->log_level;

    sg_state = ProcessEventState_Max;
    retval = ACK_NO_ERROR;

// There is no error path, so no need for this label.
//cleanup0:
    FreeAcpResponse_();
    return retval;
}

#endif // ACK_LOGGING

static void FreeAcpResponse_(void)
{
    HeapletFree(MEM_RECEIVED_ENCODED_ACP_RESPONSE);
}

static void FreeDirectiveDocument_(void)
{
    HeapletFree(MEM_RECEIVED_DIRECTIVE_INFO);
    HeapletFree(MEM_RECEIVED_BINARY_DOCUMENT);
}
