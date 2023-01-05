/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_color_temperature_controller.h"
#include "ack_core_common.h"
#include "ack_core_directives.h"
#include "ack_core_document_wrappers.h"
#include "ack_user_config.h"
#include "ack_generated_ack.h"
#include "ack_generated_alexa_color_temperature_controller.h"

#ifdef ACK_COLOR_TEMPERATURE_CONTROLLER

static bool ProcessSetDirective_(const ACKDirectiveParams_t* pDocument);
static void ProcessAdjustDirective_(const ACKDirectiveParams_t* pDocument, void (*pUserCallback)(int32_t));

// Calls user-supplied routine(s) to process the directive.
ACKError_t ProcessColorTemperatureControllerDirective(const ACKDirectiveParams_t* pDocument)
{
    bool result;

    switch (pDocument->Name)
    {
    case ack_alexa_color_temperature_controller_directive_name_set_color_temperature:
        result = ProcessSetDirective_(pDocument);
        break;

    case ack_alexa_color_temperature_controller_directive_name_increase_color_temperature:
        ProcessAdjustDirective_(
            pDocument,
            ACKUser_OnColorTemperatureControllerIncreaseTemperatureDirective);
        result = true;
        break;

    case ack_alexa_color_temperature_controller_directive_name_decrease_color_temperature:
        ProcessAdjustDirective_(
            pDocument,
            ACKUser_OnColorTemperatureControllerDecreaseTemperatureDirective);
        result = true;
        break;

    default:
        result = false;
    }

    return result ? ACK_NO_ERROR : ACK_ERR_PROTOCOL;
}

static bool ProcessSetDirective_(const ACKDirectiveParams_t* pDocument)
{
    int value;

    if (!ACKDoc_GetInt(
        &pDocument->PayloadObject,
        ack_alexa_color_temperature_controller_set_color_temperature_directive_payload_object_member_color_temperature_in_kelvin,
        &value))
    {
        return false;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnColorTemperatureControllerSetTemperatureDirective(pDocument->CorrelationId, value);

    ACK_REENTRY_SHIELDS_DOWN;

    return true;
}

static void ProcessAdjustDirective_(const ACKDirectiveParams_t* pDocument, void (*pUserCallback)(int32_t))
{
    ACK_REENTRY_SHIELDS_UP;

    pUserCallback(pDocument->CorrelationId);

    ACK_REENTRY_SHIELDS_DOWN;
}

ACKError_t ACK_AddColorTemperatureControllerProperty(
    const ACKStateCommon_t* pCommon,
    uint32_t valueInKelvin)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_color_temperature_controller,
        NULL, // No instance for this capability
        ack_alexa_color_temperature_controller_property_name_color_temperature_in_kelvin,
        &propertyObject)

        && ACKDoc_AppendInt64(&propertyObject, ack_property_object_member_value, valueInKelvin);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // def ACK_COLOR_TEMPERATURE_CONTROLLER
