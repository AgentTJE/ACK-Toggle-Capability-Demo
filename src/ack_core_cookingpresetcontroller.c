/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_core_common.h"
#include "ack_core_cooking_common.h"
#include "ack_core_cooking_mode.h"
#include "ack_core_directives.h"
#include "ack_core_document_wrappers.h"
#include "ack_core_food_item.h"
#include "ack_user_config.h"
#include "ack_cooking.h"
#include "ack_generated_ack.h"
#include "ack_generated_alexa_cooking_preset_controller.h"

#ifdef ACK_COOKING_PRESET_CONTROLLER

// Calls user-supplied routine to process the directive.
ACKError_t ProcessCookingPresetControllerDirective(const ACKDirectiveParams_t* pDocument)
{
    ACKCookingDirectiveParams_t params;
    CookingDirectiveMembers members = {0};

    // The only directive in this capability is cook-by-preset.
    switch (pDocument->Name)
    {
    case ack_alexa_cooking_preset_controller_directive_name_cook_by_preset:
        break;

    default:
        return ACK_ERR_PROTOCOL;
    }

    // Preset name and cooking mode required.
    // Food item optional.
    #define _REQUIRED_FIELDS \
        CDFP_PRESET_PRESENT_AND_REQUIRED |\
        CDFP_MODE_PRESENT |\
        CDFP_FOODITEM_PRESENT |\
        CDFP_FOODDONENESS_PRESENT

    members.Preset = ack_alexa_cooking_preset_controller_cook_by_preset_payload_object_member_preset_name;
    members.Mode = ack_alexa_cooking_preset_controller_cook_by_preset_payload_object_member_cooking_mode;
    members.FoodItem = ack_alexa_cooking_preset_controller_cook_by_preset_payload_object_member_food_item;
    members.FoodDoneness = ack_alexa_cooking_preset_controller_cook_by_preset_payload_object_member_requested_food_doneness;

    if (!CookingExtractDirectiveParams(pDocument, _REQUIRED_FIELDS, &members, &params))
    {
        return ACK_ERR_PROTOCOL;
    }
    
    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnCookingPresetControllerCookByPresetDirective(pDocument->CorrelationId, &params);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

ACKError_t ACK_AddCookingPresetControllerPresetNameProperty(
    const ACKStateCommon_t* pCommon,
    const char* pPresetName)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_cooking_preset_controller,
        NULL, // No instance for this capability
        ack_alexa_cooking_preset_controller_property_name_preset_name,
        &propertyObject)
        
        && ACKDoc_AppendString(&propertyObject, ack_property_object_member_value, pPresetName);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

ACKError_t ACK_AddCookingPresetControllerRequestedFoodDonenessProperty(
    const ACKStateCommon_t* pCommon,
    ack_food_doneness foodDoneness)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_cooking_preset_controller,
        NULL, // No instance for this capability
        ack_alexa_cooking_preset_controller_property_name_requested_food_doneness,
        &propertyObject)

        && ACKDoc_AppendInt(&propertyObject, ack_property_object_member_value, foodDoneness);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // def ACK_COOKING_PRESET_CONTROLLER
