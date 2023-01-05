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
#include "ack_mode_controller.h"
#include "ack_user_config.h"
#include "ack_generated_alexa_mode_controller.h"

#ifdef ACK_MODE_CONTROLLER

// Calls user-supplied routine to process the directive.
ACKError_t ProcessModeControllerDirective(const ACKDirectiveParams_t* pDocument)
{
    bool isDelta = false;
    unsigned valueMember = 0;
    int mode;

    switch (pDocument->Name)
    {
    case ack_alexa_mode_controller_directive_name_set_mode:
        valueMember = ack_alexa_mode_controller_set_mode_directive_payload_object_member_mode;
        isDelta = false;
        break;

    case ack_alexa_mode_controller_directive_name_adjust_mode:
        valueMember = ack_alexa_mode_controller_adjust_mode_directive_payload_object_member_mode_delta;
        isDelta = true;
        break;
    
    default:
        return ACK_ERR_PROTOCOL;
    }

    if (!ACKDoc_GetInt(&pDocument->PayloadObject, valueMember, &mode))
    {
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnModeControllerDirective(pDocument->CorrelationId, pDocument->Instance, isDelta, mode);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

ACKError_t ACK_AddModeControllerProperty(
    uint32_t instance,
    const ACKStateCommon_t* pCommon,
    uint32_t mode)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_mode_controller,
        &instance,
        ack_alexa_mode_controller_property_name_mode,
        &propertyObject)
    
        && ACKDoc_AppendInt(&propertyObject, ack_property_object_member_value, mode);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // def ACK_MODE_CONTROLLER
