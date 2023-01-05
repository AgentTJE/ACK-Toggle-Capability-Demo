/*
 * Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */


#include "ack.h"
#include "ack_core_endpoint_id.h"
#include <stdint.h>

#ifdef ACK_COMPOUND_DEVICE

typedef struct _EndpointEntry_t
{
    uint16_t EndpointId;
    int32_t CorrelationId;
}
EndpointEntry_t;

EndpointEntry_t sg_endpointsMap[ACK_COMPOUND_DEVICE_ARRAY_SIZE] = {{0,0}};
uint8_t sg_lastIndex = 0;

uint16_t ACK_GetEndpointId(int32_t correlationId)
{
    // start search using the latest index.
    // most likely it's going to be the corrent one.
    uint8_t index = sg_lastIndex;

    do
    {
        if (sg_endpointsMap[index].CorrelationId == correlationId)
        {
            return sg_endpointsMap[index].EndpointId;
        }

        // if not found, loop over indexes going backwards.
        if (index > 0)
        {
            --index;
        }
        else
        {
            index = ACK_COMPOUND_DEVICE_ARRAY_SIZE - 1;
        }
    }
    // search until full loop is completed
    while (index != sg_lastIndex);

    return ACK_COMPOUND_DEVICE_DEFAULT_ENDPOINT;
}

void SetEndpointId(int32_t correlationId, uint16_t endpointId)
{
    EndpointEntry_t entry = {endpointId, correlationId};

    ++sg_lastIndex;
    sg_lastIndex %= ACK_COMPOUND_DEVICE_ARRAY_SIZE;

    sg_endpointsMap[sg_lastIndex] = entry;
}

#endif