/*
 * Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_CORE_FOOD_DONENESS_H__
#define __ACK_CORE_FOOD_DONENESS_H__

#include "ack.h"
#include "ack_cooking.h"
#include "ack_core_directives.h"
#include "ack_generated_alexa_cooking_preset_controller.h"
#include "ack_generated_food_doneness.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

bool FoodDonenessLoad(
    const ACKDocContainer_t* pObject,
    const ack_food_doneness* pFoodDoneness);

#ifdef __cplusplus
}
#endif

#endif // ndef __ACK_CORE_FOOD_DONENESS_H__