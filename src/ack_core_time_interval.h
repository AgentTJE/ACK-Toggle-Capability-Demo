/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_CORE_TIME_INTERVAL_H__
#define __ACK_CORE_TIME_INTERVAL_H__

#include "ack.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

bool TimeIntervalSave(
    const ACKDocContainer_t* pTo,
    unsigned member,
    uint64_t startTime, // epoch ms
    uint64_t endTime,
    uint64_t durationMilliseconds);

bool TimeIntervalLoad(
    const ACKDocContainer_t* pFrom,
    unsigned member,
    uint64_t* pStartTime, // epoch ms
    uint64_t* pEndTime,
    uint64_t* pDurationMilliseconds);

#ifdef __cplusplus
}
#endif

#endif // ndef __ACK_CORE_TIME_INTERVAL_H__
