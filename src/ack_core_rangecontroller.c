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
#include "ack_range_controller.h"
#include "ack_user_config.h"
#include "ack_generated_alexa_range_controller.h"

#ifdef ACK_RANGE_CONTROLLER

// Calls user-supplied routine to process the directive.
ACKError_t ProcessRangeControllerDirective(const ACKDirectiveParams_t* pDocument)
{
    bool isDelta = false;
    unsigned valueMember = 0;
    bool isDefaultDelta = false;
    double value;

    switch (pDocument->Name)
    {
    case ack_alexa_range_controller_directive_name_set_range_value:
        valueMember = ack_alexa_range_controller_set_value_directive_payload_object_member_range_value;
        isDelta = false;
        break;

    case ack_alexa_range_controller_directive_name_adjust_range_value:
        valueMember
            = ack_alexa_range_controller_adjust_value_directive_payload_object_member_range_value_delta;
        isDelta = true;
        break;
    
    default:
        return ACK_ERR_PROTOCOL;
    }

    if (!ACKDoc_GetDouble(&pDocument->PayloadObject, valueMember, &value)
        || (isDelta
            && !ACKDoc_GetBoolean(
                &pDocument->PayloadObject,
                ack_alexa_range_controller_adjust_value_directive_payload_object_member_range_value_delta_default,
                &isDefaultDelta)))
    {
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnRangeControllerDirective(
        pDocument->CorrelationId,
        pDocument->Instance,
        isDelta,
        value,
        isDefaultDelta);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

ACKError_t ACK_AddRangeControllerProperty(
    uint32_t instance,
    const ACKStateCommon_t* pCommon,
    double value)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_range_controller,
        &instance,
        ack_alexa_range_controller_property_name_range_value,
        &propertyObject)
    
        && ACKDoc_AppendDouble(&propertyObject, ack_property_object_member_value, value);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // def ACK_RANGE_CONTROLLER
