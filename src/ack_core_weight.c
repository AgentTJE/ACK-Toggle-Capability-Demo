/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_weight.h"
#include "ack_generated_weight.h"

bool WeightLoad(const ACKDocContainer_t* pFrom, ack_weight_unit* pUnit, float* pQuantity)
{
    float value;
    int unit;

    if (!ACKDoc_GetFloat(pFrom, ack_weight_object_member_value, &value)
        || !ACKDoc_GetInt(pFrom, ack_weight_object_member_unit, &unit))
    {
        return false;
    }

    *pUnit = (ack_weight_unit)unit;
    *pQuantity = value;
    return true;
}

bool WeightSave(const ACKDocContainer_t* pTo, ack_weight_unit unit, float quantity)
{
    return ACKDoc_AppendFloat(pTo, ack_weight_object_member_value, quantity)
        && ACKDoc_AppendInt(pTo, ack_weight_object_member_unit, unit);
}
