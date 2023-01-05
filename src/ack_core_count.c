/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_count.h"

bool CountLoad(const ACKDocContainer_t* pFrom, ack_count_unit* pUnit, float* pQuantity)
{
    float value;
    int unit;

    if (!ACKDoc_GetFloat(pFrom, ack_count_object_member_value, &value))
    {
        return false;
    }

    // Unit is optional for counts.
    *pUnit = (ack_count_unit)(ACKDoc_GetInt(pFrom, ack_count_object_member_unit, &unit) ? unit : 0);
    *pQuantity = value;
    return true;
}

bool CountSave(const ACKDocContainer_t* pTo, ack_count_unit unit, float quantity)
{
    return ACKDoc_AppendFloat(pTo, ack_count_object_member_value, quantity)
        && ((0 == unit) || ACKDoc_AppendInt(pTo, ack_count_object_member_unit, unit));
}
