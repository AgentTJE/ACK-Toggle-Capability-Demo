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
#include "ack_power_controller.h"
#include "ack_user_config.h"
#include "ack_generated_alexa_power_controller.h"
#include "ack_generated_power_state.h"

#ifdef ACK_POWER_CONTROLLER

ACKError_t ProcessPowerControllerDirective(const ACKDirectiveParams_t* pDocument)
{
    bool powerOn;

    // There are two directives: turn-on, and turn-off. There's no payload for either.
    switch (pDocument->Name)
    {
    case ack_alexa_power_controller_directive_name_turn_off:
        powerOn = false;
        break;
    case ack_alexa_power_controller_directive_name_turn_on:
        powerOn = true;
        break;
    default:
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnPowerControllerDirective(pDocument->CorrelationId, powerOn);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

ACKError_t ACK_AddPowerControllerProperty(
    const ACKStateCommon_t* pCommon,
    bool powerOn)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
            pCommon,
            ack_namespace_alexa_power_controller,
            NULL, // No instance for this capability
            ack_alexa_power_controller_property_name_power_state,
            &propertyObject)
        && ACKDoc_AppendInt(
            &propertyObject,
            ack_property_object_member_value,
            powerOn ? ack_power_state_on : ack_power_state_off);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // def ACK_POWER_CONTROLLER
