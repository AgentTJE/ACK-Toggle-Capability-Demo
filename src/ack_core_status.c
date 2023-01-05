/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "acp.pb.h"
#include "ack.h"
#include "ack_core_common.h"
#include "ack_core_heaplet.h"
#include "ack_user_platform.h"
#include "ack_user_module.h"
#include <inttypes.h>
#include <stdint.h>

// These represent the posix epoch most recently returned from a get-status ACP command (in seconds),
// and the local tick count (elapsed milliseconds since boot) at the time that most recent get-status
// ACP command completed. These are used by ACK_GetApproximateTime.
uint32_t g_realTimeEpochBaseSeconds = 0;
uint32_t g_realTimeTickBase = 0;

void ACK_GetStatus(ACKStatus_t* pStatus)
{
    ACK_REENTRY_CHECK;

    ACKModule_GetStatus(pStatus);

    // Store the information that allows ACK_GetApproximateTime to work. In case of error sending the
    // get-status command, or if the epoch value wasn't known, pStatus->EpochSeconds is 0,
    // it will be ignored causing time to rely on a previous call.
    if (0 != pStatus->EpochSeconds)
    {
        if (g_realTimeEpochBaseSeconds / 60 != pStatus->EpochSeconds / 60)
        {
            // print time every minute
            ACK_DEBUG_PRINT_I("Approximate epoch: %"PRIu32" seconds", pStatus->EpochSeconds);
        }

        g_realTimeEpochBaseSeconds = pStatus->EpochSeconds;
        g_realTimeTickBase = ACKPlatform_TickCount();
    }
}

uint64_t ACK_GetApproximateTime(void)
{
    // Note: this relies heavily on the lifecycle code (AckProcess) calling ACK_GetStatus frequently enough
    // to not have the tick count wrap (49.7 days). That's a safe bet because by design there isn't any way
    // to consume this codebase other than by calling ACK_Process from the main loop.
    return (g_realTimeEpochBaseSeconds && g_realTimeTickBase)
        ? ((1000 * (uint64_t)g_realTimeEpochBaseSeconds) + (ACKPlatform_TickCount() - g_realTimeTickBase))
        : 0;
}
