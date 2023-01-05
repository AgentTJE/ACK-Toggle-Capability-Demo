/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_food_item.h"
#include "ack_core_item_quantity.h"
#include "ack_generated_food_item.h"

bool FoodItemLoad(
    const ACKDocContainer_t* pFrom,
    unsigned member,
    const char** ppFoodName,
    ACKItemQuantity_t* pFoodQuantity)
{
    ACKDocContainer_t foodItemObject;
    const char* pFoodName;

    *ppFoodName = NULL;

    // Note: we don't attempt to load the food category.
    if (!ACKDoc_GetObject(pFrom, member, &foodItemObject)
        || !ItemQuantityLoad(&foodItemObject, ack_food_item_object_member_food_quantity, pFoodQuantity))
    {
        return false;
    }

    // Food name is optional.
    if (!ACKDoc_GetString(&foodItemObject, ack_food_item_object_member_food_name, &pFoodName))
    {
        pFoodName = NULL;
    }

    *ppFoodName = pFoodName;
    return true;
}

bool FoodItemSave(
    const ACKDocContainer_t* pTo,
    unsigned member,
    const char* pFoodName,
    const ACKItemQuantity_t* pFoodQuantity)
{
    ACKDocContainer_t foodItemObject;

    // Note: we don't attempt to set the food category.
    if (!ACKDoc_AppendObject(pTo, member, &foodItemObject)
        || !ItemQuantitySave(&foodItemObject, ack_food_item_object_member_food_quantity, pFoodQuantity)
        || (pFoodName && !ACKDoc_AppendString(&foodItemObject, ack_food_item_object_member_food_name, pFoodName)))
    {
        return false;
    }

    return true;
}
