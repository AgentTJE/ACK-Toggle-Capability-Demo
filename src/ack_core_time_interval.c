/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_time_interval.h"
#include "ack_generated_time_interval.h"

bool TimeIntervalSave(
    const ACKDocContainer_t* pTo,
    unsigned member,
    uint64_t startTime, // epoch ms
    uint64_t endTime,
    uint64_t durationMilliseconds)
{
    ACKDocContainer_t timeIntervalObject;

    if (!ACKDoc_AppendObject(pTo, member, &timeIntervalObject)

        || (startTime 
            && !ACKDoc_AppendInt64(&timeIntervalObject, ack_time_interval_object_member_start, (int64_t)startTime))

        || (endTime
            && !ACKDoc_AppendInt64(&timeIntervalObject, ack_time_interval_object_member_end, (int64_t)endTime))

        || (durationMilliseconds
            && !ACKDoc_AppendInt64(
                &timeIntervalObject,
                ack_time_interval_object_member_duration,
                (int64_t)durationMilliseconds)))
    {
        return false;
    }

    return true;
}

bool TimeIntervalLoad(
    const ACKDocContainer_t* pFrom,
    unsigned member,
    uint64_t* pStartTime, // epoch ms
    uint64_t* pEndTime,
    uint64_t* pDurationMilliseconds)
{
    ACKDocContainer_t timeIntervalObject;

    ACK_STATIC_ASSERT(sizeof(*pStartTime) == sizeof(int64_t));
    ACK_STATIC_ASSERT(sizeof(*pEndTime) == sizeof(int64_t));
    ACK_STATIC_ASSERT(sizeof(*pDurationMilliseconds) == sizeof(int64_t));

    if (!ACKDoc_GetObject(pFrom, member, &timeIntervalObject))
    {
        return false;
    }

    // Start, end. duration are all optional, and 0 indicates absence.
    *pStartTime = 0;
    *pEndTime = 0;
    *pDurationMilliseconds = 0;

    (void)ACKDoc_GetInt64(&timeIntervalObject, ack_time_interval_object_member_start, (int64_t*)pStartTime);

    (void)ACKDoc_GetInt64(&timeIntervalObject, ack_time_interval_object_member_end, (int64_t*)pEndTime);

    (void)ACKDoc_GetInt64(
        &timeIntervalObject,
        ack_time_interval_object_member_duration,
        (int64_t*)pDurationMilliseconds);

    return true;
}
