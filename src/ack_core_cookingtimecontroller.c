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
#include "ack_core_power_level.h"
#include "ack_core_temperature.h"
#include "ack_user_config.h"
#include "ack_generated_ack.h"
#include "ack_generated_alexa_cooking_time_controller.h"

#ifdef ACK_COOKING_TIME_CONTROLLER

// Forward declarations.
static ACKError_t ProcessCookByTimeDirective_(const ACKDirectiveParams_t* pDocument);
static ACKError_t ProcessAdjustCookTimeDirective_(const ACKDirectiveParams_t* pDocument);

// Calls user-supplied routine to process the directive.
ACKError_t ProcessCookingTimeControllerDirective(const ACKDirectiveParams_t* pDocument)
{
    ACKError_t retval;

    switch (pDocument->Name)
    {
    case ack_alexa_cooking_time_controller_directive_name_cook_by_time:
        retval = ProcessCookByTimeDirective_(pDocument);
        break;

    case ack_alexa_cooking_time_controller_directive_name_adjust_cook_time:
        retval = ProcessAdjustCookTimeDirective_(pDocument);
        break;

    default:
        retval = ACK_ERR_PROTOCOL;
        break;
    }

    return retval;
}

static ACKError_t ProcessCookByTimeDirective_(const ACKDirectiveParams_t* pDocument)
{
    ACKCookingDirectiveParams_t params;
    CookingDirectiveMembers members = {0};

    // Cook time required.
    // Cooking mode, food item, power level, and target temperature optional.
    #define _REQUIRED_FIELDS \
        CDFP_DURATION_PRESENT_AND_REQUIRED |\
        CDFP_MODE_PRESENT |\
        CDFP_FOODITEM_PRESENT |\
        CDFP_POWERLEVEL_PRESENT |\
        CDFP_TEMPERATURE_PRESENT

    members.Duration = ack_alexa_cooking_time_controller_cook_by_time_payload_object_member_cook_time;
    members.Mode = ack_alexa_cooking_time_controller_cook_by_time_payload_object_member_cooking_mode;
    members.FoodItem = ack_alexa_cooking_time_controller_cook_by_time_payload_object_member_food_item;
    members.PowerLevel = ack_alexa_cooking_time_controller_cook_by_time_payload_object_member_cooking_power_level;
    members.Temperature
        = ack_alexa_cooking_time_controller_cook_by_time_payload_object_member_target_cooking_temperature;

    if (!CookingExtractDirectiveParams(pDocument, _REQUIRED_FIELDS, &members, &params))
    {
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnCookingTimeControllerCookByTimeDirective(pDocument->CorrelationId, &params);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

static ACKError_t ProcessAdjustCookTimeDirective_(const ACKDirectiveParams_t* pDocument)
{
    int64_t delta;

    if (!ACKDoc_GetInt64(
        &pDocument->PayloadObject,
        ack_alexa_cooking_time_controller_adjust_cook_time_payload_object_member_cook_time_delta,
        &delta))
    {
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnCookingTimeControllerAdjustCookTimeDirective(pDocument->CorrelationId, delta);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

ACKError_t ACK_AddCookingTimeControllerRequestedCookTimeProperty(
    const ACKStateCommon_t* pCommon,
    uint64_t durationMilliseconds)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_cooking_time_controller,
        NULL, // No instance for this capability
        ack_alexa_cooking_time_controller_property_name_requested_cook_time,
        &propertyObject)

        && ACKDoc_AppendInt64(&propertyObject, ack_property_object_member_value, durationMilliseconds);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

ACKError_t ACK_AddCookingTimeControllerCookingPowerLevelProperty(
    const ACKStateCommon_t* pCommon,
    ACKCookingPowerLevelKind_t powerLevelKind,
    int32_t integralPowerLevel,
    ack_enumerated_power_level enumeratedPowerLevel)
{
    ACKDocContainer_t propertyObject;
    ACKCookingPowerLevel_t level;
    bool result;

    ACK_REENTRY_CHECK;

    if (ACK_COOKING_POWER_LEVEL_ENUMERATED == powerLevelKind)
    {
        level.Enumerated = enumeratedPowerLevel;
    }
    else
    {
        level.Integral = integralPowerLevel;
    }

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_cooking_time_controller,
        NULL, // No instance for this capability
        ack_alexa_cooking_time_controller_property_name_cooking_power_level,
        &propertyObject)

        && PowerLevelSave(&propertyObject, ack_property_object_member_value, powerLevelKind, &level);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // def ACK_COOKING_TIME_CONTROLLER
