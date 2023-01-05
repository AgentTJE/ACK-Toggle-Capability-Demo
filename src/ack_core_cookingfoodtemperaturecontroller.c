/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_cooking.h"
#include "ack_core_common.h"
#include "ack_core_cooking_common.h"
#include "ack_core_cooking_mode.h"
#include "ack_core_directives.h"
#include "ack_core_document_wrappers.h"
#include "ack_core_food_item.h"
#include "ack_core_temperature.h"
#include "ack_user_config.h"
#include "ack_generated_ack.h"
#include "ack_generated_alexa_cooking_food_temperature_controller.h"

#ifdef ACK_COOKING_FOOD_TEMPERATURE_CONTROLLER

// Calls user-supplied routine to process the directive.
ACKError_t ProcessFoodTemperatureControllerCookingDirective(const ACKDirectiveParams_t* pDocument)
{
    ACKCookingDirectiveParams_t params;
    CookingDirectiveMembers members = {0};

    // The only directive in this capability is cook-by-food-temperature.
    switch (pDocument->Name)
    {
    case ack_alexa_cooking_food_temperature_controller_directive_name_cook_by_food_temperature:
        break;

    default:
        return ACK_ERR_PROTOCOL;
    }

    // Temperature required.
    // Mode, food item optional.
    #define _REQUIRED_FIELDS \
        CDFP_TEMPERATURE_PRESENT | CDFP_TEMPERATURE_REQUIRED |\
        CDFP_MODE_PRESENT |\
        CDFP_FOODITEM_PRESENT

    members.Temperature
        = ack_alexa_cooking_food_temperature_controller_cook_by_food_temperature_payload_object_member_target_food_temperature;
    members.Mode
        = ack_alexa_cooking_food_temperature_controller_cook_by_food_temperature_payload_object_member_cooking_mode;
    members.FoodItem
        = ack_alexa_cooking_food_temperature_controller_cook_by_food_temperature_payload_object_member_food_item;

    if (!CookingExtractDirectiveParams(pDocument, _REQUIRED_FIELDS, &members, &params))
    {
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnCookingFoodTemperatureControllerCookByFoodTemperatureDirective(
        pDocument->CorrelationId,
        &params);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

ACKError_t ACK_AddTargetFoodTemperatureProperty(
    const ACKStateCommon_t* pCommon,
    ack_temperature_scale scale,
    float temperature)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_cooking_food_temperature_controller,
        NULL, // No instance for this capability
        ack_alexa_cooking_food_temperature_controller_property_name_target_food_temperature,
        &propertyObject)

        && TemperatureSave(&propertyObject, ack_property_object_member_value, scale, temperature);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // def ACK_COOKING_FOOD_TEMPERATURE_CONTROLLER
