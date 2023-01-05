/*
 * Copyright 2018-2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

// Functionality for validation.
//
// Validation Markers flow from HMCU to the module. A Marker is in the following form:
//
//  @~^OPEN*ordinal*tick\r\n
//  Marker name\r\n
//  Optional string1\r\n
//  Optional string2\r\n
//  ...
//  @~^CLOSE*ordinal\r\n
//
// The ordinal is a monotonically increasing integer that the receiver can use to detect missing/corrupt
// messages.

#include "ack.h"
#include "ack_core_validation.h"
#include "ack_user_config.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#ifdef ACK_VALIDATION

static uint32_t sg_validationMessageOrdinal = 0;

// Sends a marker, consisting of an opening line, one or more lines of body, and a closing line.
// The other side of the conversation re-assembles the complete message.
void ValidationSendMarker(const char* pName, ...)
{
    va_list arglist;
    const char* pString;
    uint32_t nowTicks;
    char buffer[16];

    ++sg_validationMessageOrdinal;

    nowTicks = ACKPlatform_TickCount();

    ACKPlatform_DebugPrint("@~^OPEN*");
    snprintf(buffer, sizeof(buffer), "%"PRIu32, sg_validationMessageOrdinal);
    ACKPlatform_DebugPrint(buffer);

    snprintf(buffer, sizeof(buffer), "*%"PRIu32, nowTicks);
    ACKPlatform_DebugPrint(buffer);
    ACKPlatform_DebugPrint("\r\n");

    ACKPlatform_DebugPrint(pName);
    ACKPlatform_DebugPrint("\r\n");

    va_start(arglist, pName);

    pString = va_arg(arglist, const char*);
    while (pString)
    {
        ACKPlatform_DebugPrint(pString);
        ACKPlatform_DebugPrint("\r\n");
        pString = va_arg(arglist, const char*);
    }

    va_end(arglist);

    ACKPlatform_DebugPrint("@~^CLOSE*");
    snprintf(buffer, sizeof(buffer), "%"PRIu32, sg_validationMessageOrdinal);
    ACKPlatform_DebugPrint(buffer);
    ACKPlatform_DebugPrint("\r\n");
}

#endif // def ACK_VALIDATION
