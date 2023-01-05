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
#include "ack_core_communication.h"
#include "ack_core_event_builder.h"
#include "ack_core_fixed_memory.h"
#include "ack_core_heaplet.h"
#include "ack_core_protobuf_utilities.h"
#include <inttypes.h>

#ifdef ACK_ENABLE_ASSERT

// Bitwise indicator of which categories of guarded code are currently active.
volatile uint8_t g_reentryGuard = 0;

void ReentryFail(const char* pFunctionName)
{
    ACK_DEBUG_PRINT_C(
        "The SDK detected a call to %s while another call to the SDK is still active. "
        "You must ensure that you do not call SDK functions re-entrantly, such as from a timer service routine, "
        "or, if you are using an RTOS, simultaneously from concurrent RTOS tasks. "
        "You must also not call SDK routines other than ACK_Complete* from within your handler callbacks "
        "(e.g. routines whose names begin with 'ACKUser_On').",
        pFunctionName);
    ACK_AssertionFailed("re-entry check");
}

#endif

#ifdef ACK_ENABLE_ASSERT

void (*ACK_AssertionFailedHook)(void);

void ACK_AssertionFailed(const char* pCondition)
{
#ifndef ACK_DEBUG_PRINT_LEVEL
#error ACK_ENABLE_ASSERT requires debug print be enabled but ACK_DEBUG_PRINT_LEVEL is not defined
#endif
    ACK_DEBUG_PRINT_C("***** ASSERTION FAILED: %s.", pCondition);

    while (1)
    {
        if (ACK_AssertionFailedHook)
        {
            ACK_AssertionFailedHook();
        }
    }
}

#endif // ACK_ENABLE_ASSERT
