/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_color_controller.h"
#include "ack_core_common.h"
#include "ack_core_directives.h"
#include "ack_core_document_wrappers.h"
#include "ack_user_config.h"
#include "ack_generated_ack.h"
#include "ack_generated_alexa_color_controller.h"
#include "ack_generated_color.h"

#ifdef ACK_COLOR_CONTROLLER

ACKError_t ProcessColorControllerDirective(const ACKDirectiveParams_t* pDocument)
{
    ACKDocContainer_t colorObject;
    double hue;
    double saturation;
    double brightness;

    // There's only one directive: set-color.
    if ((ack_alexa_color_controller_directive_name_set_color != pDocument->Name)
        || !ACKDoc_GetObject(
                &pDocument->PayloadObject,
                ack_alexa_color_controller_set_color_directive_payload_object_member_color,
                &colorObject)
        || !ACKDoc_GetDouble(&colorObject, ack_color_object_member_hue, &hue)
        || !ACKDoc_GetDouble(&colorObject, ack_color_object_member_saturation, &saturation)
        || !ACKDoc_GetDouble(&colorObject, ack_color_object_member_brightness, &brightness))
    {
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnColorControllerDirective(pDocument->CorrelationId, hue, saturation, brightness);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

ACKError_t ACK_AddColorControllerProperty(
    const ACKStateCommon_t* pCommon,
    double hue,
    double saturation,
    double brightness)
{
    ACKDocContainer_t propertyObject;
    ACKDocContainer_t valueObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
            pCommon,
            ack_namespace_alexa_color_controller,
            NULL, // No instance for this capability
            ack_alexa_color_controller_property_name_color,
            &propertyObject)
        && ACKDoc_AppendObject(&propertyObject, ack_property_object_member_value, &valueObject)
        && ACKDoc_AppendDouble(&valueObject, ack_color_object_member_hue, hue)
        && ACKDoc_AppendDouble(&valueObject, ack_color_object_member_saturation, saturation)
        && ACKDoc_AppendDouble(&valueObject, ack_color_object_member_brightness, brightness);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // def ACK_COLOR_CONTROLLER
