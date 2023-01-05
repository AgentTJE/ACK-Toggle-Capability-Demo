/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_core_common.h"
#include "ack_core_communication.h"
#include "ack_core_document.h"
#include "ack_core_document_wrappers.h"
#include "ack_core_event_builder.h"
#include "ack_core_fixed_memory.h"
#include "ack_core_heaplet.h"
#include "ack_core_protobuf_utilities.h"
#include "acp.pb.h"
#include "ack_user_module.h"
#include <inttypes.h>

static int32_t sg_correlationId;
static uint32_t sg_instance;

#ifdef ACK_COMPOUND_DEVICE
extern uint16_t g_eventEndpointId;
#endif

#ifdef ACK_EVENT_BUILDER_DIAG

static enum EventBuilderState
{
    EventBuilderState_NotBuilding,
    EventBuilderState_BuildingNoProperties,
    EventBuilderState_BuildingWithProperties,
    EventBuilderState_BuildingWithChangeReportPayloadProperties,
}
sg_state;

bool EventBuilderIsEventBeingBuilt(void)
{
    return EventBuilderState_NotBuilding != sg_state;
}

#endif // def ACK_EVENT_BUILDER_DIAG

// Begins building an event.
bool EventBuilderStart(
    int32_t correlationId,
    ack_namespace eventNamespace,
    unsigned eventName,
    uint32_t instance,
    unsigned flags)
{
    EventBuilderEnd();

#ifdef ACK_COMPOUND_DEVICE
    if (correlationId != 0)
    {
        // for change reports the expectation that it's being set outside of this function
        g_eventEndpointId = ACK_GetEndpointId(correlationId);
    }
#endif

    if (!CreateEventDocument(
        eventNamespace,
        eventName,
        (flags & EVENT_FLAG_NO_CONTEXT) ? CEDF_OMIT_CONTEXT : 0))
    {
        return false;
    }

    sg_correlationId = correlationId;
    sg_instance = instance;

#ifdef ACK_EVENT_BUILDER_DIAG
    sg_state = EventBuilderState_BuildingNoProperties;
    if (flags & EVENT_FLAG_CAN_HAVE_PROPERTIES)
    {
        sg_state = EventBuilderState_BuildingWithProperties;

        if (flags & EVENT_FLAG_CAN_HAVE_CHANGE_REPORT_PAYLOAD_PROPERTIES)
        {
            sg_state = EventBuilderState_BuildingWithChangeReportPayloadProperties;
        }
    }
#endif

    return true;
}

// Ends tracking the building of an event. Idempotent.
void EventBuilderEnd(void)
{
    // Free all memory blocks associated with building events.
    HeapletFree(MEM_OUTBOUND_BINARY_DOCUMENT);

#ifdef ACK_EVENT_BUILDER_DIAG
    sg_state = EventBuilderState_NotBuilding;
#endif
}

// Diagnostic to determine whether adding a property to the event being built is allowed.
#ifdef ACK_EVENT_BUILDER_DIAG

void EventBuilderCheckCanAddProperty(bool changeReportPayloadProperty)
{
    if (changeReportPayloadProperty)
    {
        ACK_ASSERT(EventBuilderState_BuildingWithChangeReportPayloadProperties == sg_state);
    }

    ACK_ASSERT((EventBuilderState_BuildingWithProperties == sg_state)
        || (EventBuilderState_BuildingWithChangeReportPayloadProperties == sg_state));
}

#endif

// Sends an event with payload from MEM_OUTBOUND_BINARY_DOCUMENT.
ACKError_t EventBuilderSend(void)
{
    ProtobufCallbackField_t desc;
    V3AVS_AvsEvent* pEvent;
    ACKError_t retval;

    ACK_DEBUG_PRINT_I("Sending event.");

    FixedMemoryWipeAcp();    

    g_fixedAcpMemory.Command.which_type = acp_cmd_send_alexa_event_obj_tag;
    pEvent = &g_fixedAcpMemory.Command.type.send_alexa_event_obj.event;

    pEvent->v3_namespace = ACK_DOM_BINARY_FORM_ENVELOPE_NAMESPACE;
    pEvent->instance = sg_instance;
    pEvent->correlation_id = sg_correlationId;
    pEvent->payload.funcs.encode = ProtobufEncodeCallbackField;
    pEvent->payload.arg = &desc;

    // Trim the document before sending.
    DocumentMakeReadOnly(MEM_OUTBOUND_BINARY_DOCUMENT);

    desc.Value = HeapletAccess(MEM_OUTBOUND_BINARY_DOCUMENT);
    desc.Length = HeapletGetSize(MEM_OUTBOUND_BINARY_DOCUMENT);

    retval = ACKModule_SendEvent(pEvent);

    EventBuilderEnd();

    return retval;
}
