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
#include "ack_core_heaplet.h"
#include "ack_module_diagnostics.h"
#include "acp.pb.h"
#include "ack_user_module.h"

// Retrieves device type info from the connectivity module.
ACKError_t ACK_GetDeviceType(char* pDeviceType, size_t deviceTypeBufferSize)
{
    ACK_REENTRY_CHECK;

    return ACKModule_GetDeviceType(pDeviceType, deviceTypeBufferSize);
}

#ifdef ACK_MODULE_DIAGNOSTICS

// Retrieves provisioning info from the connectivity module.
ACKError_t ACK_GetProvisioningInfo(acp_response_provisioning_provisioning_state* pState)
{
    ACK_REENTRY_CHECK;

    return ACKModule_GetProvisioningInfo(pState);
}

// Retrieves hardware info from the connectivity module.
ACKError_t ACK_GetHardwareInfo(char* pDsn, size_t dsnBufferSize)
{
    ACK_REENTRY_CHECK;

    return ACKModule_GetHardwareInfo(pDsn, dsnBufferSize);
}

// Retrieves firmware version info from the connectivity module.
ACKError_t ACK_GetFirmwareVersion(
    uint32_t* pProtocolNumber,
    uint32_t* pBuildNumber,
    char* pIncrementalVersion,
    size_t incrementalVersionBufferSize)
{
    ACK_REENTRY_CHECK;

    return ACKModule_GetFirmwareVersion(
        pProtocolNumber,
        pBuildNumber,
        pIncrementalVersion,
        incrementalVersionBufferSize);
}

#endif // def ACK_MODULE_DIAGNOSTICS
