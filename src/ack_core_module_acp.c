/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_common.h"
#include "ack_core_communication.h"
#include "ack_core_fixed_memory.h"
#include "ack_core_heaplet.h"
#include "ack_core_protobuf_utilities.h"
#include "ack_core_status.h"
#include "ack_metrics.h"
#include "acp.pb.h"
#include "pb_decode.h"

// MEM_RECEIVED_ENCODED_ACP_RESPONSE is allocated within the function
// Caller is responsible for freeing all blocks in all scenarios.
static ACKError_t SendAcpCommandAndGetResponse_(const acp_cmd* pCommand, acp_response* pResponse);
static ACKError_t SendAcpCommand_(const acp_cmd* pCommand);
static ACKError_t GetAcpResponse_(acp_response* pResponse);
static void FreeEncodedResponse_(void);
static bool CheckGenericResponse_(const acp_response* pResponse);

static ACKError_t CreateAndSendProvisioningCommand_(uint32_t submessageTag);
static bool SkipToEncodedResponsePayload_(uint32_t tag1, uint32_t optionalTag2, size_t *pOffset);

#ifdef ACK_METRICS

// There is a MQTT limit to the size of packets we can send, so the encoded packet must be limited.
#define METRICS_MAX_ENCODED_SIZE 768

// Note: it is safe to call those routines when in a handler callback (ACKUser_Onxxx).
ACKError_t ACKModule_SendMetric(
    acp_cmd_record_dem_metric_Type type,
    size_t datapointCount,
    const ACKMetricDatapoint_t* pDatapoints)
{
    acp_metric_datapoint* pAcpDatapoints = NULL;
    ProtobufCallbackField_t* pTextValues = NULL;
    ACKError_t retval;
    ProtobufEncodeArrayCallbackData_t* pEncodeData;
    size_t size;
    size_t i;

    // NOTE: no reentry check. We use the alt heap blocks for sending out the ACP command and reading and
    // processing the result.

    ACK_DEBUG_PRINT_I("Sending metric.");

    // Allocate a single heaplet block to store a ProtobufEncodeArrayCallbackData_t, one acp_metric_datapoint
    // for each datapoint, and one ProtobufCallbackField_t for each discrete/textual metric -- in that order
    // within the block. First calculate the required size, then allocate the block and set up pointers.
    size = sizeof(ProtobufEncodeArrayCallbackData_t) + (sizeof(acp_metric_datapoint) * datapointCount);

    for (i = 0; i < datapointCount; ++i)
    {
        if ((acp_metric_datapoint_Type_DISCRETE == pDatapoints[i].Type)
            && (NULL != pDatapoints[i].Value.Discrete))
        {
            size += sizeof(ProtobufCallbackField_t);
        }
    }

    pEncodeData = (ProtobufEncodeArrayCallbackData_t*)HeapletSetSize(MEM_OUTBOUND_LOG_AND_METRICS_DATA, size);
    if (NULL == pEncodeData)
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    memset(pEncodeData, 0, size);

    pAcpDatapoints = (acp_metric_datapoint*)(pEncodeData + 1);
    pTextValues = (ProtobufCallbackField_t*)(pAcpDatapoints + datapointCount);

    // Context.properties field.
    pEncodeData->Data = pAcpDatapoints;
    pEncodeData->ElementCount = datapointCount;
    pEncodeData->ElementSize = sizeof(acp_metric_datapoint);

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_record_dem_metric_obj_tag;
    g_fixedAcpMemory.Command.type.record_dem_metric_obj.metric_type = type;
    g_fixedAcpMemory.Command.type.record_dem_metric_obj.datapoints.arg = pEncodeData;
    g_fixedAcpMemory.Command.type.record_dem_metric_obj.datapoints.funcs.encode = ProtobufEncodeArrayCallback;

    for (i = 0; i < datapointCount; i++)
    {
        pAcpDatapoints->data_type = pDatapoints->Type;

        ACK_SAFE_COPY_STRING(pAcpDatapoints->name, pDatapoints->Name);

        switch (pDatapoints->Type)
        {
        case acp_metric_datapoint_Type_GAUGE:
            pAcpDatapoints->number = pDatapoints->Value.Gauge;
            break;

        case acp_metric_datapoint_Type_DISCRETE:
            if (pDatapoints->Value.Discrete != NULL)
            {
                pTextValues->Value = (uint8_t*)pDatapoints->Value.Discrete;
                pTextValues->Length = strlen(pDatapoints->Value.Discrete);

                pAcpDatapoints->text.arg = pTextValues;
                pAcpDatapoints->text.funcs.encode = ProtobufEncodeCallbackField;

                ++pTextValues;
            }

            break;

        // The only legal remaining type is COUNTER. If something else is specified, trigger an
        // assertion failure to call attention to the programming error.
        // Use counter as the default for this because interpreting the union as an integer is safe,
        // whereas interpreting a string would be unsafe (if we made discrete the default).
        case acp_metric_datapoint_Type_COUNTER:
        default:
            ACK_ASSERT(acp_metric_datapoint_Type_COUNTER == pDatapoints->Type);
            pAcpDatapoints->counter = pDatapoints->Value.Counter;
            break;
        }

        ++pAcpDatapoints;
        ++pDatapoints;
    }

    // Calculate the encoded size. If the encoded size is too big (more than METRICS_MAX_ENCODED_SIZE bytes),
    // we should spit out an error. There is a MQTT limit to the size of packets we can send, so the encoded
    // packet cannot be too large.
    // TODO: Inside the SendAcpCommandAndGetResponse_ we calculate ProtobufGetEncodedSize again, we should
    // figure out a way to make only one calculation
    size = ProtobufGetEncodedSize(acp_cmd_fields, &g_fixedAcpMemory.Command);

    if ((0 == size) || (size > METRICS_MAX_ENCODED_SIZE))
    {
        ACK_DEBUG_PRINT_E(
            "Unable to determine encoded metrics size, or size (%u bytes) exceeds %u bytes.",
            (unsigned)size, // case because data types might be different sizes
            METRICS_MAX_ENCODED_SIZE);
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    retval = SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, &g_fixedAcpMemory.Response);

    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Error encoding and sending command, err %u", retval);
        goto cleanup0;
    }

    if (!CheckGenericResponse_(&g_fixedAcpMemory.Response))
    {
        ACK_DEBUG_PRINT_E("Unexpected response to command.");
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();
    HeapletFree(MEM_OUTBOUND_LOG_AND_METRICS_DATA);
    return retval;
}

#endif /* ACK_METRICS */

ACKError_t ACKModule_SendEvent(V3AVS_AvsEvent* pEvent)
{
    return SendAcpCommand_(&g_fixedAcpMemory.Command);
}

ACKError_t ACKModule_GetResponse(acp_response* pResponse)
{
    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_get_next_event_obj_tag;

    return SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, pResponse);
}

ACKError_t ACKModule_GetEncodedDirective(
    acp_event_alexa_directive_received* pDirective,
    uint8_t const** ppData,
    size_t* pDataLen)
{
    ProtobufCallbackField_t field = {0};
    bool protoResult;

    pDirective->directive.payload.funcs.decode = ProtobufDecodeCallbackField;
    pDirective->directive.payload.arg = &field;

    // 'Decode' here means that we get a pointer to the still-encoded submessage, within the original
    // encoded event.
    protoResult = ProtobufDecodeSubmessage(
        HeapletAccess(MEM_RECEIVED_ENCODED_ACP_RESPONSE),
        HeapletGetSize(MEM_RECEIVED_ENCODED_ACP_RESPONSE),
        acp_response_alexa_directive_received_obj_tag,
        acp_event_alexa_directive_received_fields,
        pDirective);
    if (!protoResult)
    {
        ACK_DEBUG_PRINT_E("Unable to decode directive submessage.");
        return ACK_ERR_PROTOCOL;
    }

    *ppData = field.Value;
    *pDataLen = field.Length;

    return ACK_NO_ERROR;
}

// Sends a simple provisioning command (no payload) and gets a response into
// MEM_RECEIVED_ENCODED_ACP_RESPONSE and fixed acp_response memory.
// Caller must always clean up.
static ACKError_t CreateAndSendProvisioningCommand_(uint32_t submessageTag)
{
    ACKError_t retval;

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_provisioning_obj_tag;
    g_fixedAcpMemory.Command.type.provisioning_obj.which_command = submessageTag;

    // Send command and get response into MEM_RECEIVED_ENCODED_ACP_RESPONSE and fixed acp_response memory.
    retval = SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, &g_fixedAcpMemory.Response);
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Unable to send provisioning command, err %u.", retval);
        goto cleanup0;
    }

    retval = ACK_NO_ERROR;

cleanup0:
    return retval;
}

// Assuming MEM_RECEIVED_ENCODED_ACP_RESPONSE contains a valid command response which itself
// is known to contain a specific submessage, skips to the first byte of the submessage's encoded payload.
// Returns an offset within MEM_RECEIVED_ENCODED_ACP_RESPONSE where the caller should start decoding.
static bool SkipToEncodedResponsePayload_(uint32_t tag1, uint32_t optionalTag2, size_t *pOffset)
{
    pb_istream_t stream;
    size_t streamSize;

    streamSize = HeapletGetSize(MEM_RECEIVED_ENCODED_ACP_RESPONSE);

    stream = pb_istream_from_buffer(HeapletAccess(MEM_RECEIVED_ENCODED_ACP_RESPONSE), streamSize);

    if (!ProtobufSkipToEncodedSubmessagePayload(&stream, tag1))
    {
        return false;
    }

    if (optionalTag2)
    {
        if (!ProtobufSkipToEncodedSubmessagePayload(&stream, optionalTag2))
        {
            return false;
        }
    }

    *pOffset = streamSize - stream.bytes_left;
    return true;
}

#ifdef ACK_MODULE_DIAGNOSTICS

ACKError_t ACKModule_GetProvisioningInfo(acp_response_provisioning_provisioning_state* pState)
{
    size_t encodedPayloadOffset;
    ACKError_t retval;

    ACK_DEBUG_PRINT_I("Sending get-provisioning-info command.");

    retval = CreateAndSendProvisioningCommand_(acp_cmd_provisioning_get_provisioning_state_obj_tag);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Try to skip to the encoded payload for the get-device-type response. If we can't, then
    // we got back an unexpected or corrupted response.
    if (!SkipToEncodedResponsePayload_(
        acp_response_provisioning_obj_tag,
        acp_response_provisioning_provisioning_info_obj_tag,
        &encodedPayloadOffset))
    {
        ACK_DEBUG_PRINT_E("Incorrect response to get-provisioning-info diagnostic command.");
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    *pState = g_fixedAcpMemory.Response.type.provisioning_obj.response.provisioning_info_obj.state;

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();
    return retval;
}

ACKError_t ACKModule_GetHardwareInfo(char* pDsn, size_t dsnBufferSize)
{
    size_t encodedPayloadOffset;
    ACKError_t retval;
    ProtobufCallbackField_t dsnField = {0};
    bool protoResult;

    ACK_DEBUG_PRINT_I("Sending get-hardware-info command.");

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_get_ack_hardware_info_obj_tag;

    // Send command and get response into MEM_RECEIVED_ENCODED_ACP_RESPONSE and fixed acp_response memory.
    retval = SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, &g_fixedAcpMemory.Response);
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Unable to send provisioning command, err %u.", retval);
        goto cleanup0;
    }

    // Try to skip to the encoded payload for the get-hardware-info response. If we can't, then
    // we got back an unexpected or corrupted response.
    if (!SkipToEncodedResponsePayload_(
        acp_response_get_ack_hardware_info_obj_tag,
        0,
        &encodedPayloadOffset))
    {
        ACK_DEBUG_PRINT_E("Incorrect response to get-hardware-info diagnostic command.");
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Response.type.get_ack_hardware_info_obj.dsn.arg = &dsnField;
    g_fixedAcpMemory.Response.type.get_ack_hardware_info_obj.dsn.funcs.decode = ProtobufDecodeCallbackField;

    protoResult = ProtobufDecode(
        (uint8_t*)HeapletAccess(MEM_RECEIVED_ENCODED_ACP_RESPONSE) + encodedPayloadOffset,
        HeapletGetSize(MEM_RECEIVED_ENCODED_ACP_RESPONSE) - encodedPayloadOffset,
        acp_response_get_ack_hardware_info_fields,
        &g_fixedAcpMemory.Response.type.get_ack_hardware_info_obj);
    if (!protoResult)
    {
        ACK_DEBUG_PRINT_E("Unable to decode get-hardware-info diagnostic command result.");
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    // Not much point in calling this function if the DSN buffer can't get anything written
    // into it (at least one character and a nul terminator).
    ACK_ASSERT(dsnBufferSize > 1);
    if (dsnField.Length >= dsnBufferSize)
    {
        dsnField.Length = dsnBufferSize - 1;
    }

    memcpy(pDsn, dsnField.Value, dsnField.Length);
    pDsn[dsnField.Length] = 0;

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();
    return retval;
}

ACKError_t ACKModule_GetFirmwareVersion(
    uint32_t* pProtocolNumber,
    uint32_t* pBuildNumber,
    char* pIncrementalVersion,
    size_t incrementalVersionBufferSize)
{
    size_t encodedPayloadOffset;
    ACKError_t retval;
    ProtobufCallbackField_t incrementalVersionField = {0};
    bool protoResult;

    ACK_DEBUG_PRINT_I("Sending get-firmware-version command.");

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_get_firmware_version_obj_tag;

    retval = SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, &g_fixedAcpMemory.Response);
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Unable to send get-firmware-version; err %u.", retval);
        goto cleanup0;
    }

    // Try to skip to the encoded payload for the get-firmware-version response. If we can't, then
    // we got back an unexpected or corrupted response.
    if (!SkipToEncodedResponsePayload_(
        acp_response_get_firmware_version_obj_tag,
        0,
        &encodedPayloadOffset))
    {
        ACK_DEBUG_PRINT_E("Incorrect response to get-firmware-version diagnostic command.");
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Response.type.get_firmware_version_obj.incremental_version.arg
        = &incrementalVersionField;
    g_fixedAcpMemory.Response.type.get_firmware_version_obj.incremental_version.funcs.decode
        = ProtobufDecodeCallbackField;

    protoResult = ProtobufDecode(
        (uint8_t*)HeapletAccess(MEM_RECEIVED_ENCODED_ACP_RESPONSE) + encodedPayloadOffset,
        HeapletGetSize(MEM_RECEIVED_ENCODED_ACP_RESPONSE) - encodedPayloadOffset,
        acp_response_get_firmware_version_fields,
        &g_fixedAcpMemory.Response.type.get_firmware_version_obj);
    if (!protoResult)
    {
        ACK_DEBUG_PRINT_E("Unable to decode get-firmware-version diagnostic command result.");
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    // Not much point in calling this function if the incremental version buffer can't get anything written
    // into it (at least one character and a nul terminator).
    ACK_ASSERT(incrementalVersionBufferSize > 1);
    if (incrementalVersionField.Length >= incrementalVersionBufferSize)
    {
        incrementalVersionField.Length = incrementalVersionBufferSize - 1;
    }

    memcpy(pIncrementalVersion, incrementalVersionField.Value, incrementalVersionField.Length);
    pIncrementalVersion[incrementalVersionField.Length] = 0;

    *pProtocolNumber = g_fixedAcpMemory.Response.type.get_firmware_version_obj.protocol_number;
    *pBuildNumber = g_fixedAcpMemory.Response.type.get_firmware_version_obj.build_number;

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();
    return retval;
}

#endif /* ACK_MODULE_DIAGNOSTICS */

ACKError_t ACKModule_GetDeviceType(char* pDeviceType, size_t deviceTypeBufferSize)
{
    size_t encodedPayloadOffset;
    ACKError_t retval;
    bool protoResult;
    ProtobufCallbackField_t deviceTypeField = {0};

    ACK_DEBUG_PRINT_I("Sending get-device-type command.");

    retval = CreateAndSendProvisioningCommand_(acp_cmd_provisioning_get_device_type_obj_tag);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Try to skip to the encoded payload for the get-device-type response. If we can't, then
    // we got back an unexpected or corrupted response.
    if (!SkipToEncodedResponsePayload_(
        acp_response_provisioning_obj_tag,
        acp_response_provisioning_device_type_obj_tag,
        &encodedPayloadOffset))
    {
        ACK_DEBUG_PRINT_E("Incorrect response to get-device-type diagnostic command.");
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Response.type.provisioning_obj.response.device_type_obj.device_type.arg
        = &deviceTypeField;
    g_fixedAcpMemory.Response.type.provisioning_obj.response.device_type_obj.device_type.funcs.decode
        = ProtobufDecodeCallbackField;

    protoResult = ProtobufDecode(
        (uint8_t*)HeapletAccess(MEM_RECEIVED_ENCODED_ACP_RESPONSE) + encodedPayloadOffset,
        HeapletGetSize(MEM_RECEIVED_ENCODED_ACP_RESPONSE) - encodedPayloadOffset,
        acp_response_provisioning_device_type_fields,
        &g_fixedAcpMemory.Response.type.provisioning_obj.response.device_type_obj);
    if (!protoResult)
    {
        ACK_DEBUG_PRINT_E("Unable to decode get-manufacturing-info diagnostic command result.");
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    // Not much point in calling this function if we can't at least get a terminating nul in there.
    ACK_ASSERT(0 != deviceTypeBufferSize);
    if (deviceTypeField.Length >= deviceTypeBufferSize)
    {
        deviceTypeField.Length = deviceTypeBufferSize - 1;
    }

    memcpy(pDeviceType, deviceTypeField.Value, deviceTypeField.Length);
    pDeviceType[deviceTypeField.Length] = 0;

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();
    return retval;
}

#ifdef ACK_LOGGING

static void DebugPrint(acp_log_level logLevel, const char* pMessage)
{
    switch (logLevel)
    {
    case acp_log_level_info:
        ACK_DEBUG_PRINT_I("LOG: %s", pMessage);
        break;
    case acp_log_level_error:
        ACK_DEBUG_PRINT_E("LOG: %s", pMessage);
        break;
    default:
        ACK_ASSERT(acp_log_level_debug == logLevel);
        ACK_DEBUG_PRINT_D("LOG: %s", pMessage);
        break;
    }
}

// Note carefully that this routine uses "AltHeapletBlocks" common routines to send the write-log ACP command
// and receive the response. This is a set of heaplet tags that when used will not move or overwrite
// existing blocks that may be in use when the user call ACK_WriteLog/ACK_WriteLogFormatted. Because of this,
// it is safe to call those routines when in a handler callback (ACKUser_Onxxx).
ACKError_t ACKModule_WriteLog(acp_log_level logLevel, const char* pComponent, const char* pMessage)
{
    ACKError_t retval;

    // Put these on the stack for now. If there are more, or more memory is needed, consider changing to
    // putting these in the heaplet.
    ProtobufCallbackField_t componentStringCallback = {0};
    ProtobufCallbackField_t messageStringCallback = {0};

    // Print the log message to the debug port also.
    DebugPrint(logLevel, pMessage);

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_write_log_obj_tag;
    g_fixedAcpMemory.Command.type.write_log_obj.log_level = logLevel;
    g_fixedAcpMemory.Command.type.write_log_obj.component.arg = &componentStringCallback;
    g_fixedAcpMemory.Command.type.write_log_obj.component.funcs.encode = ProtobufEncodeCallbackField;

    // Note: the ACK Connectivity Module ignores characters after the 63rd in the component string
    // (which has a fixed length of 64 bytes, including the nul terminator), but we don't try to enforce
    // that here.
    componentStringCallback.Value = (const uint8_t*)pComponent;
    componentStringCallback.Length = strlen(pComponent);

    g_fixedAcpMemory.Command.type.write_log_obj.message.arg = &messageStringCallback;
    g_fixedAcpMemory.Command.type.write_log_obj.message.funcs.encode = ProtobufEncodeCallbackField;

    messageStringCallback.Value = (const uint8_t*)pMessage;
    messageStringCallback.Length = strlen(pMessage);

    retval = SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, &g_fixedAcpMemory.Response);
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Error encoding and sending command, err %u", retval);
        goto cleanup0;
    }

    if (!CheckGenericResponse_(&g_fixedAcpMemory.Response))
    {
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();
    return retval;
}

ACKError_t ACKModule_SubmitLogs(void)
{
    ACKError_t retval;

    ACK_DEBUG_PRINT_I("Sending submit logs command");

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_submit_logs_obj_tag;

    retval = SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, &g_fixedAcpMemory.Response);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    if (!CheckGenericResponse_(&g_fixedAcpMemory.Response))
    {
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();
    return retval;
}

#endif /* ACK_LOGGING */

ACKError_t ACKModule_HostConfigWrite(uint64_t firmwareVersion)
{
    ACKError_t retval;

    ACK_DEBUG_PRINT_I("Sending write host config command");

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_write_host_config_obj_tag;

    g_fixedAcpMemory.Command.type.write_host_config_obj.firmware_version = firmwareVersion;
    g_fixedAcpMemory.Command.type.write_host_config_obj.host_info_count = 0;

    retval = SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, &g_fixedAcpMemory.Response);
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Error encoding and sending command, err %u", retval);
        goto cleanup0;
    }

    if (!CheckGenericResponse_(&g_fixedAcpMemory.Response))
    {
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();
    return retval;
}

ACKError_t ACKModule_FactoryResetInitiate(void)
{
    ACKError_t retval;

    ACK_DEBUG_PRINT_I("Sending trigger factory reset command");

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_trigger_factory_reset_obj_tag;

    retval = SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, &g_fixedAcpMemory.Response);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    if (!CheckGenericResponse_(&g_fixedAcpMemory.Response))
    {
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();
    return retval;
}

ACKError_t ACKModule_UserGuidedSetupInitiate(void)
{
    ACKError_t retval;

    ACK_DEBUG_PRINT_I("Sending initiate user-guided setup command to the module.");

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_enter_setup_mode_obj_tag;

    // For now the only supported type is Frustration-Free Setup.
    g_fixedAcpMemory.Command.type.enter_setup_mode_obj.mode = acp_cmd_enter_setup_mode_registration_type_ffs;
    g_fixedAcpMemory.Command.type.enter_setup_mode_obj.security
        = acp_cmd_enter_setup_mode_security_type_insecure;

    // Send command and get response into MEM_RECEIVED_ENCODED_ACP_RESPONSE and fixed acp_response memory.
    retval = SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, &g_fixedAcpMemory.Response);
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Error encoding and sending initiate user-guided setup command, err %u", retval);
        goto cleanup0;
    }

    if (!CheckGenericResponse_(&g_fixedAcpMemory.Response))
    {
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();
    return retval;
}

void ACKModule_GetStatus(ACKStatus_t* pStatus)
{
    ACKError_t retval;
    acp_response_get_ack_status* pStatusResponse;

    ACK_DEBUG_PRINT_D("Sending get-status command.");

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_get_ack_status_obj_tag;

    // Send command and get response into MEM_RECEIVED_ENCODED_ACP_RESPONSE and fixed acp_response memory.
    retval = SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, &g_fixedAcpMemory.Response);
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Send acp_cmd and get and decode acp_response failed, err %u", retval);
        goto cleanup0;
    }

    if (g_fixedAcpMemory.Response.which_type == acp_response_generic_obj_tag)
    {
        ACK_DEBUG_PRINT_E(
            "Received generic error response %u",
            g_fixedAcpMemory.Response.type.generic_obj.code);
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    if (g_fixedAcpMemory.Response.which_type != acp_response_get_ack_status_obj_tag)
    {
        ACK_DEBUG_PRINT_E(
            "Incorrect response. Expected get_ProcessEventState_status response. Instead got %u.",
            g_fixedAcpMemory.Response.which_type);
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    // Fill out the output struct.
    pStatusResponse = &g_fixedAcpMemory.Response.type.get_ack_status_obj;

    pStatus->HH = (int)pStatusResponse->current_time.hh;
    pStatus->MM = (int)pStatusResponse->current_time.mm;
    pStatus->SS = (int)pStatusResponse->current_time.ss;
    pStatus->EpochSeconds = pStatusResponse->epoch_seconds;

    pStatus->SubStateInfo.InSetupMode.SetupStage = acp_setup_stages_none;
    pStatus->SubStateInfo.InSetupMode.IsUserGuidedSetupActive = 0;
    pStatus->SubStateInfo.InSetupMode.IsBarcodeScanSetupActive = 0;
    pStatus->SubStateInfo.InSetupMode.IsZeroTouchSetupActive = 0;

    if (pStatusResponse->setup_stage == acp_setup_stages_timeout)
    {
        // Timeout indicates ACK_NOT_REGISTERED and takes priority over other values.
        pStatus->ConnectivityState = ACK_NOT_REGISTERED;
    }
    else if (pStatusResponse->setup_mode)
    {
        pStatus->ConnectivityState = ACK_IN_SETUP_MODE;

        // Set the setup type if present by prioritized order and determined by
        // the lowest set bit in the setup_types field.
        pStatus->SubStateInfo.InSetupMode.IsUserGuidedSetupActive =
            (pStatusResponse->setup_types & acp_setup_types_ugs) ? 1 : 0;
        pStatus->SubStateInfo.InSetupMode.IsBarcodeScanSetupActive =
            (pStatusResponse->setup_types & acp_setup_types_bcs) ? 1 : 0;
        pStatus->SubStateInfo.InSetupMode.IsZeroTouchSetupActive =
            (pStatusResponse->setup_types & acp_setup_types_zts) ? 1 : 0;

        pStatus->SubStateInfo.InSetupMode.SetupStage = pStatusResponse->setup_stage;
    }
    else
    {
        pStatus->ConnectivityState = (acp_wifi_mode_connected == pStatusResponse->wifi_mode)
            ? ACK_CONNECTED_TO_ALEXA
            : ACK_NOT_CONNECTED_TO_ALEXA;

        pStatus->SubStateInfo.NotConnectedToAlexa.DisconnectionReason = pStatusResponse->wifi_disconnection_reason;
        pStatus->SubStateInfo.NotConnectedToAlexa.IsDistressBeaconing = pStatusResponse->is_distress_beaconing;
    }

    pStatus->ErrorCode = pStatusResponse->error_code;

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();

    if (ACK_NO_ERROR != retval)
    {
        pStatus->HH = -1;
        pStatus->MM = -1;
        pStatus->SS = -1;
        pStatus->EpochSeconds = 0;

        pStatus->ConnectivityState = ACK_CONNECTIVITY_UNKNOWN;

        pStatus->ErrorCode = acp_response_error_codes_no_error;
    }
}

#ifdef ACK_HOST_FIRMWARE_UPDATE

ACKError_t ACKModule_OtaDownload(
    uint32_t readOffset,
    uint32_t maxReadLength,
    uint8_t* pBytes,
    uint32_t* pActualLength)
{
    acp_response_read_host_fw_update_bytes* pFirmwareUpdateResponse;
    ProtobufCallbackField_t field = {0};
    bool protoResult;
    ACKError_t retval;

    ACK_DEBUG_PRINT_I("Sending get host ota bytes command");

    FixedMemoryWipeAcp();
    g_fixedAcpMemory.Command.which_type = acp_cmd_read_host_fw_update_bytes_obj_tag;
    g_fixedAcpMemory.Command.type.read_host_fw_update_bytes_obj.length = maxReadLength;
    g_fixedAcpMemory.Command.type.read_host_fw_update_bytes_obj.offset = readOffset;

    retval = SendAcpCommandAndGetResponse_(&g_fixedAcpMemory.Command, &g_fixedAcpMemory.Response);
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Unable to send read host OTA update command, err %u", retval);
        goto cleanup0;
    }

    if (acp_response_read_host_fw_update_bytes_obj_tag != g_fixedAcpMemory.Response.which_type)
    {
        ACK_DEBUG_PRINT_E(
            "Unexpected read host fw update response type %u",
            g_fixedAcpMemory.Response.which_type);
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    // Lower level response message decode.
    pFirmwareUpdateResponse = &g_fixedAcpMemory.Response.type.read_host_fw_update_bytes_obj;
    pFirmwareUpdateResponse->bytes.funcs.decode = ProtobufDecodeCallbackField;
    pFirmwareUpdateResponse->bytes.arg = &field;

    // 'Decode' here means that we get a pointer to the still-encoded submessage, within the original
    // encoded event.
    protoResult = ProtobufDecodeSubmessage(
        HeapletAccess(MEM_RECEIVED_ENCODED_ACP_RESPONSE),
        HeapletGetSize(MEM_RECEIVED_ENCODED_ACP_RESPONSE),
        acp_response_read_host_fw_update_bytes_obj_tag,
        acp_response_read_host_fw_update_bytes_fields,
        pFirmwareUpdateResponse);

    if (!protoResult)
    {
        ACK_DEBUG_PRINT_E("Unable to decode read host firmware bytes submessage");
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    if (field.Length > maxReadLength)
    {
        ACK_DEBUG_PRINT_E(
            "That's funny; read host firmware ota has %u bytes but we requested a max of %u bytes",
            (unsigned)field.Length,
            (unsigned)maxReadLength);
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    memcpy(pBytes, field.Value, field.Length);
    *pActualLength = field.Length;

    retval = ACK_NO_ERROR;

cleanup0:
    FreeEncodedResponse_();
    return retval;
}

#endif // def ACK_HOST_FIRMWARE_UPDATE

static ACKError_t SendAcpCommandAndGetResponse_(const acp_cmd* pCommand, acp_response* pResponse)
{
    ACKError_t retval;

    retval = SendAcpCommand_(pCommand);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    retval = GetAcpResponse_(pResponse);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    return retval;
}

static ACKError_t SendAcpCommand_(const acp_cmd* pCommand)
{
    size_t encodedSize;
    void *pEncodedData;
    ACKError_t retval;

    encodedSize = ProtobufGetEncodedSize(acp_cmd_fields, pCommand);

    pEncodedData = HeapletSetSize(MEM_OUTBOUND_ENCODED_ACP_CMD, encodedSize);
    if (NULL == pEncodedData)
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    ACK_DEBUG_PRINT_D("Encoding v3 event fields, encoded size is %u.", (unsigned)encodedSize);

    if (!ProtobufEncode(pEncodedData, encodedSize, acp_cmd_fields, pCommand))
    {
        ACK_DEBUG_PRINT_E(
            "Unable to encode the send Alexa response command. Buffer size is %u.",
            (unsigned)encodedSize);
        retval = ACK_ERR_INTERNAL;
        goto cleanup0;
    }

    ACKPlatform_DrainRead();

    retval = CommsSendAcpProtobufCmd(pEncodedData, encodedSize);
    if (retval != ACK_NO_ERROR)
    {
        ACK_DEBUG_PRINT_E("Unable to send Alexa response command, err: %u.", retval);
        goto cleanup0;
    }

    // Done.
    retval = ACK_NO_ERROR;

cleanup0:
    // Free blocks we used internally. OK to do this even if never allocated (such as in an error path),
    // or already freed.
    HeapletFree(MEM_OUTBOUND_ENCODED_ACP_CMD);
    return retval;
}

static ACKError_t GetAcpResponse_(acp_response* pResponse)
{
    ACKError_t retval;

    // Read the response into MEM_RECEIVED_ENCODED_ACP_RESPONSE, and decode it into fixed
    // acp_response memory.
    retval = CommsGetAcpResponse();
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    FixedMemoryWipeAcp();

    if (!ProtobufDecode(
        HeapletAccess(MEM_RECEIVED_ENCODED_ACP_RESPONSE),
        HeapletGetSize(MEM_RECEIVED_ENCODED_ACP_RESPONSE),
        acp_response_fields,
        pResponse))
    {
        ACK_DEBUG_PRINT_E("Unable to decode acp_response.");
        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    // Don't free the encoded response in the success case, because the caller may need to extract
    // some additional binary data from it.
    if (ACK_NO_ERROR != retval)
    {
        HeapletFree(MEM_RECEIVED_ENCODED_ACP_RESPONSE);
    }

    return retval;
}

static void FreeEncodedResponse_(void)
{
    HeapletFree(MEM_RECEIVED_ENCODED_ACP_RESPONSE);
}

// Checks a response to ensure it's a generic response.
static bool CheckGenericResponse_(const acp_response* pResponse)
{
    if (acp_response_generic_obj_tag != pResponse->which_type)
    {
        ACK_DEBUG_PRINT_E("Expected generic response; got %u.", pResponse->which_type);
        return false;
    }

    if (acp_response_generic_error_no_error != pResponse->type.generic_obj.code)
    {
        ACK_DEBUG_PRINT_E("Generic response indicates error %u.", pResponse->type.generic_obj.code);
        return false;
    }

    ACK_DEBUG_PRINT_D("Got generic response indicating no error.");
    return true;
}
