/*
 * Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_food_doneness.h"
#include "ack_generated_ack.h"
#include "ack_generated_food_doneness.h"

bool FoodDonenessLoad(
    const ACKDocContainer_t* pObject,
    const ack_food_doneness* pFoodDoneness)
{
    bool result;

    result = false;
    result = ACKDoc_GetInt(
        pObject, 
        ack_alexa_cooking_preset_controller_cook_by_preset_payload_object_member_requested_food_doneness, 
        (int*)pFoodDoneness
        );

    return result;
}
