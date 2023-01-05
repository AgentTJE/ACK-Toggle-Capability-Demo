/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_power_level.h"
#include "ack_generated_enumerated_power_level.h"
#include "ack_generated_integral_power_level.h"
#include "ack_generated_power_level.h"

bool PowerLevelLoad(
    const ACKDocContainer_t* pObject,
    unsigned member,
    ACKPowerLevelKind_t* pKind,
    ACKPowerLevel_t* pLevel)
{
    ACKDocContainer_t powerLevelObject;
    ACKDocContainer_t valueObject;
    int type;
    int value;
    bool result;

    result = ACKDoc_GetObject(pObject, member, &powerLevelObject)
        && ACKDoc_GetInt(&powerLevelObject, ack_power_level_object_member_type, &type)

        // Polymorphic object; members of the object selected by @type are in a 'value' subobject.
        && ACKDoc_GetObject(&powerLevelObject, ack_power_level_object_member_value, &valueObject);

    if (result)
    {
        switch (type)
        {
        case ack_power_level_object_type_enumerated_power_level:
            result = ACKDoc_GetInt(&valueObject, ack_enumerated_power_level_object_member_value, &value);

            if (result)
            {
                *pKind = ACK_POWER_LEVEL_ENUMERATED;
                pLevel->Enumerated = (ack_enumerated_power_level)value;
            }
            break;

        case ack_power_level_object_type_integral_power_level:
            result = ACKDoc_GetInt(&valueObject, ack_integral_power_level_object_member_value, &value);

            if (result)
            {
                *pKind = ACK_POWER_LEVEL_INTEGRAL;
                pLevel->Integral = value;
            }
            break;

        default:
            result = false;
            break;
        }
    }

    return result;
}

bool PowerLevelSave(
    const ACKDocContainer_t* pObject,
    unsigned member,
    ACKPowerLevelKind_t kind,
    const ACKPowerLevel_t* pLevel)
{
    ACKDocContainer_t object;
    ACKDocContainer_t valueObject;
    bool result;
    ack_power_level_object_type type = (ack_power_level_object_type)0;
    int value = 0;
    unsigned objectMember = 0;

    result = ACKDoc_AppendObject(pObject, member, &object);
    if (result)
    {
        switch (kind)
        {
        case ACK_POWER_LEVEL_ENUMERATED:
            type = ack_power_level_object_type_enumerated_power_level;
            objectMember = ack_enumerated_power_level_object_member_value;
            value = pLevel->Enumerated;
            break;

        case ACK_POWER_LEVEL_INTEGRAL:
            type = ack_power_level_object_type_integral_power_level;
            objectMember = ack_integral_power_level_object_member_value;
            value = pLevel->Integral;
            break;

        default:
            result = false;
            break;
        }

        if (result)
        {
            result = ACKDoc_AppendInt(&object, ack_power_level_object_member_type, type)

                // Polymorphic object; members of the object selected by @type go into a 'value' subobject.
                && ACKDoc_AppendObject(&object, ack_power_level_object_member_value, &valueObject)

                && ACKDoc_AppendInt(&valueObject, objectMember, value);
        }
    }

    return result;
}
