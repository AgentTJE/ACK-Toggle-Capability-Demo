/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_USER_MODULE_H__
#define __ACK_USER_MODULE_H__

#include "ack.h"
#include "ack_metrics.h"
#include "acp.pb.h"
#include "pb.h"
#include "V3AVS.pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ACK_HOST_FIRMWARE_UPDATE

ACKError_t ACKModule_OtaDownload(
    uint32_t readOffset,
    uint32_t maxReadLength,
    uint8_t* pBytes,
    uint32_t* pActualLength);

#endif // def ACK_HOST_FIRMWARE_UPDATE

ACKError_t ACKModule_UserGuidedSetupInitiate(void);

void ACKModule_GetStatus(ACKStatus_t* pStatus);

ACKError_t ACKModule_GetResponse(acp_response* pResponse);

ACKError_t ACKModule_GetEncodedDirective(
    acp_event_alexa_directive_received* pDirective,
    uint8_t const** ppData,
    size_t* pDataLen);

ACKError_t ACKModule_GetDeviceType(char* pDeviceType, size_t deviceTypeBufferSize);

#ifdef ACK_MODULE_DIAGNOSTICS

ACKError_t ACKModule_GetProvisioningInfo(acp_response_provisioning_provisioning_state* pState);

ACKError_t ACKModule_GetHardwareInfo(char* pDsn, size_t dsnBufferSize);

ACKError_t ACKModule_GetFirmwareVersion(
    uint32_t* pProtocolNumber,
    uint32_t* pBuildNumber,
    char* pIncrementalVersion,
    size_t incrementalVersionBufferSize);

#endif  /* ACK_MODULE_DIAGNOSTICS */

#ifdef ACK_LOGGING

ACKError_t ACKModule_WriteLog(acp_log_level logLevel, const char* pComponent, const char* pMessage);

ACKError_t ACKModule_SubmitLogs(void);

#endif /* ACK_LOGGING */

ACKError_t ACKModule_HostConfigWrite(uint64_t firmwareVersion);

ACKError_t ACKModule_FactoryResetInitiate(void);

ACKError_t ACKModule_SendEvent(V3AVS_AvsEvent* pEvent);

#ifdef ACK_METRICS

ACKError_t ACKModule_SendMetric(
    acp_cmd_record_dem_metric_Type type,
    size_t datapointCount,
    const ACKMetricDatapoint_t* pDatapoints);

#endif /* ACK_METRICS */

#ifdef __cplusplus
}
#endif

#endif // def __ACK_USER_MODULE_H__
