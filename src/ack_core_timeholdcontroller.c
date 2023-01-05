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
#include "ack_time_hold_controller.h"
#include "ack_user_config.h"
#include "ack_generated_alexa_time_hold_controller.h"

#ifdef ACK_TIME_HOLD_CONTROLLER

// Calls user-supplied routine to process the directive.
ACKError_t ProcessTimeHoldControllerDirective(const ACKDirectiveParams_t* pDocument)
{
    void (*pUserHandler)(int32_t);

    // There are two directives: hold and resume. There's no payload for either.
    switch (pDocument->Name)
    {
    case ack_alexa_time_hold_controller_directive_name_hold:
        pUserHandler = ACKUser_OnTimeHoldControllerHoldDirective;
        break;

    case ack_alexa_time_hold_controller_directive_name_resume:
        pUserHandler = ACKUser_OnTimeHoldControllerResumeDirective;
        break;

    default:
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    pUserHandler(pDocument->CorrelationId);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

#endif // def ACK_TIME_HOLD_CONTROLLER
