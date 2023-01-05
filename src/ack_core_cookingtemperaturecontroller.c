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
#include "ack_core_time_interval.h"
#include "ack_user_config.h"
#include "ack_generated_ack.h"
#include "ack_generated_alexa_cooking_temperature_controller.h"

#ifdef ACK_COOKING_TEMPERATURE_CONTROLLER

static ACKError_t ProcessCookByTemperatureDirective_(const ACKDirectiveParams_t* pDocument);
static ACKError_t ProcessAdjustTemperatureDirective_(const ACKDirectiveParams_t* pDocument);

// Calls user-supplied routine to process the directive.
ACKError_t ProcessCookingTemperatureControllerDirective(const ACKDirectiveParams_t* pDocument)
{
    ACKError_t retval;

    switch (pDocument->Name)
    {
    case ack_alexa_cooking_temperature_controller_directive_name_cook_by_temperature:
        retval = ProcessCookByTemperatureDirective_(pDocument);
        break;

    case ack_alexa_cooking_temperature_controller_directive_name_adjust_cooking_temperature:
        retval = ProcessAdjustTemperatureDirective_(pDocument);
        break;

    default:
        retval = ACK_ERR_PROTOCOL;
        break;
    }

    return retval;
}

static ACKError_t ProcessCookByTemperatureDirective_(const ACKDirectiveParams_t* pDocument)
{
    ACKCookingDirectiveParams_t params;
    CookingDirectiveMembers members = {0};

    // Temperature required.
    // Cooking mode and food item optional.
    #define _REQUIRED_FIELDS \
        CDFP_TEMPERATURE_PRESENT | CDFP_TEMPERATURE_REQUIRED |\
        CDFP_MODE_PRESENT |\
        CDFP_FOODITEM_PRESENT

    members.Temperature
        = ack_alexa_cooking_temperature_controller_cook_by_temperature_payload_object_member_target_cooking_temperature;
    members.Mode = ack_alexa_cooking_temperature_controller_cook_by_temperature_payload_object_member_cooking_mode;
    members.FoodItem = ack_alexa_cooking_temperature_controller_cook_by_temperature_payload_object_member_food_item;

    if (!CookingExtractDirectiveParams(pDocument, _REQUIRED_FIELDS, &members, &params))
    {
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnCookingTemperatureControllerCookByTemperatureDirective(pDocument->CorrelationId, &params);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

static ACKError_t ProcessAdjustTemperatureDirective_(const ACKDirectiveParams_t* pDocument)
{
    ack_temperature_scale scale;
    float value;

    if (!TemperatureLoad(
        &pDocument->PayloadObject,
        ack_alexa_cooking_temperature_controller_adjust_cook_temperature_payload_object_member_target_cooking_temperature_delta,
        &scale,
        &value))
    {
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnCookingTemperatureControllerAdjustCookingTemperatureDirective(
        pDocument->CorrelationId,
        value,
        scale);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

ACKError_t ACK_AddCookingTargetTemperatureProperty(
    const ACKStateCommon_t* pCommon,
    ack_temperature_scale scale,
    float temperature)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_cooking_temperature_controller,
        NULL, // No instance for this capability
        ack_alexa_cooking_temperature_controller_property_name_target_cooking_temperature,
        &propertyObject)

        && TemperatureSave(&propertyObject, ack_property_object_member_value, scale, temperature);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

// Call this to add a property representing the preheat time interval to an outbound event.
ACKError_t ACK_AddCookingPreheatTimeIntervalProperty(
    const ACKStateCommon_t* pCommon,
    uint64_t startTime, // epoch ms
    uint64_t endTime,
    uint64_t durationMilliseconds)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
            pCommon,
            ack_namespace_alexa_cooking_temperature_controller,
            NULL, // No instance for this capability
            ack_alexa_cooking_temperature_controller_property_name_preheat_time_interval,
            &propertyObject)
        && TimeIntervalSave(
            &propertyObject,
            ack_property_object_member_value,
            startTime,
            endTime,
            durationMilliseconds);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // ACK_COOKING_TEMPERATURE_CONTROLLER
