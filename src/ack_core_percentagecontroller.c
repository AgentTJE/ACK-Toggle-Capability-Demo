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
#include "ack_core_directives.h"
#include "ack_core_document_wrappers.h"
#include "ack_percentage_controller.h"
#include "ack_user_config.h"
#include "ack_generated_alexa_percentage_controller.h"

#ifdef ACK_PERCENTAGE_CONTROLLER

ACKError_t ProcessPercentageControllerDirective(const ACKDirectiveParams_t* pDocument)
{
    bool isDelta;
    unsigned payloadValueName;
    int32_t value;

    // There are two directives: adjust-percentage, set-percentage.
    switch (pDocument->Name)
    {
    case ack_alexa_percentage_controller_directive_name_adjust_percentage:
        isDelta = true;
        payloadValueName
            = ack_alexa_percentage_controller_adjust_percentage_directive_payload_object_member_percentage_delta;
        break;
    case ack_alexa_percentage_controller_directive_name_set_percentage:
        isDelta = false;
        payloadValueName = ack_alexa_percentage_controller_set_percentage_directive_payload_object_member_percentage;
        break;
    default:
        return ACK_ERR_PROTOCOL;
    }

    if (!ACKDoc_GetInt(&pDocument->PayloadObject, payloadValueName, &value))
    {
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnPercentageControllerDirective(pDocument->CorrelationId, isDelta, value);
    
    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

ACKError_t ACK_AddPercentageControllerProperty(const ACKStateCommon_t* pCommon, int32_t percentage)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_percentage_controller,
        NULL, // No instance for this capability
        ack_alexa_percentage_controller_property_name_percentage,
        &propertyObject)

        && ACKDoc_AppendInt(&propertyObject, ack_property_object_member_value, percentage);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // def ACK_PERCENTAGE_CONTROLLER
