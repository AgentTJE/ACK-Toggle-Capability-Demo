/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_core_temperature.h"
#include "ack_generated_temperature.h"

bool TemperatureSave(
    const ACKDocContainer_t* pObject,
    unsigned member,
    ack_temperature_scale scale,
    float value)
{
    ACKDocContainer_t temperatureObject;

    return ACKDoc_AppendObject(pObject, member, &temperatureObject)
        && ACKDoc_AppendInt(&temperatureObject, ack_temperature_object_member_scale, scale)
        && ACKDoc_AppendFloat(&temperatureObject, ack_temperature_object_member_value, value);
}

bool TemperatureLoad(
    const ACKDocContainer_t* pObject,
    unsigned member,
    ack_temperature_scale* pScale,
    float* pValue)
{
    ACKDocContainer_t temperatureObject;
    int scale;
    float value;

    ACK_STATIC_ASSERT(sizeof(int) == sizeof(scale));

    if (!ACKDoc_GetObject(pObject, member, &temperatureObject)
        || !ACKDoc_GetInt(&temperatureObject, ack_temperature_object_member_scale, &scale)
        || !ACKDoc_GetFloat(&temperatureObject, ack_temperature_object_member_value, &value))
    {
        return false;
    }

    *pScale = (ack_temperature_scale)scale;
    *pValue = value;
    return true;
}
