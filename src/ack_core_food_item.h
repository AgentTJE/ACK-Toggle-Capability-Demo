/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_CORE_FOOD_ITEM_H__
#define __ACK_CORE_FOOD_ITEM_H__

#include "ack.h"
#include "ack_cooking.h"
#include "ack_core_directives.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

bool FoodItemLoad(
    const ACKDocContainer_t* pFrom,
    unsigned member,
    const char** ppFoodName,
    ACKItemQuantity_t* pFoodQuantity);

bool FoodItemSave(
    const ACKDocContainer_t* pTo,
    unsigned member,
    const char* pFoodName,
    const ACKItemQuantity_t* pFoodQuantity);

#ifdef __cplusplus
}
#endif

#endif // ndef __ACK_CORE_FOOD_ITEM_H__
