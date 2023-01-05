/*
 * Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_user_platform.h"
#include "ack_debug_print.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef ACK_DEBUG_PRINT_LEVEL

const char* const c_pDebugLevelString = "DBG";
const char* const c_pInfoLevelString = "INF";
const char* const c_pWarningLevelString = "WRN";
const char* const c_pErrorLevelString = "ERR";
const char* const c_pCriticalLevelString = "CRT";

void ACK_DebugPrint(const char* pLevel, const char* pFunction, const char* pFormat, ...)
{
    char buffer[250];
    va_list argList;

    snprintf(buffer, sizeof(buffer), "%"PRIu32" [%s %s] ", ACKPlatform_TickCount(), pLevel, pFunction);
    ACKPlatform_DebugPrint(buffer);

    va_start(argList, pFormat);
    vsnprintf(buffer, sizeof(buffer), pFormat, argList);
    va_end(argList);
    ACKPlatform_DebugPrint(buffer);

    ACKPlatform_DebugPrint("\r\n");
 }

#endif