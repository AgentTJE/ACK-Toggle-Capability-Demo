/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_DEBUG_PRINT_H__
#define __ACK_DEBUG_PRINT_H__

#include "ack_user_platform.h"
#include "ack_user_config.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Debug print levels. #define ACK_DEBUG_PRINT_LEVEL to one of these in ack_user_config.h to enable
// debug output at that level and lower. Leave ACK_DEBUG_PRINT_LEVEL undefined to disable debug printing.
#define ACK_DEBUG_PRINT_LEVEL_CRITICAL 0
#define ACK_DEBUG_PRINT_LEVEL_ERROR 1
#define ACK_DEBUG_PRINT_LEVEL_WARNING 2
#define ACK_DEBUG_PRINT_LEVEL_INFO 3
#define ACK_DEBUG_PRINT_LEVEL_DEBUG 4

#define ACK_DEBUG_PRINT_BASE(_lvlstring,  _format, ...) \
    ACK_DebugPrint(_lvlstring, __FUNCTION__, _format, ##__VA_ARGS__)

#ifdef ACK_DEBUG_PRINT_LEVEL

#if ACK_DEBUG_PRINT_LEVEL >= ACK_DEBUG_PRINT_LEVEL_DEBUG
#define ACK_DEBUG_PRINT_D(_format, ...) \
    ACK_DEBUG_PRINT_BASE(c_pDebugLevelString, _format, ##__VA_ARGS__)
#else
#define ACK_DEBUG_PRINT_D(_format, ...)
#endif

#if ACK_DEBUG_PRINT_LEVEL >= ACK_DEBUG_PRINT_LEVEL_INFO
#define ACK_DEBUG_PRINT_I(_format, ...) \
    ACK_DEBUG_PRINT_BASE(c_pInfoLevelString, _format, ##__VA_ARGS__)
#else
#define ACK_DEBUG_PRINT_I(_message, ...)
#endif

#if ACK_DEBUG_PRINT_LEVEL >= ACK_DEBUG_PRINT_LEVEL_WARNING
#define ACK_DEBUG_PRINT_W(_format, ...) \
    ACK_DEBUG_PRINT_BASE(c_pWarningLevelString, _format, ##__VA_ARGS__)
#else
#define ACK_DEBUG_PRINT_W(_format, ...)
#endif

#if ACK_DEBUG_PRINT_LEVEL >= ACK_DEBUG_PRINT_LEVEL_ERROR
#define ACK_DEBUG_PRINT_E(_format, ...) \
    ACK_DEBUG_PRINT_BASE(c_pErrorLevelString, _format, ##__VA_ARGS__)
#else
#define ACK_DEBUG_PRINT_E(_format, ...)
#endif

#if ACK_DEBUG_PRINT_LEVEL >= ACK_DEBUG_PRINT_LEVEL_CRITICAL
#define ACK_DEBUG_PRINT_C(_format, ...) \
    ACK_DEBUG_PRINT_BASE(c_pCriticalLevelString, _format, ##__VA_ARGS__)
#else
#define ACK_DEBUG_PRINT_C(_format, ...)
#endif

extern const char* const c_pDebugLevelString;
extern const char* const c_pInfoLevelString;
extern const char* const c_pWarningLevelString;
extern const char* const c_pErrorLevelString;
extern const char* const c_pCriticalLevelString;

void ACK_DebugPrint(const char* pLevel, const char* pFunction, const char* pFormat, ...);

#else

#define ACK_DEBUG_PRINT_D(_format, ...)
#define ACK_DEBUG_PRINT_I(_format, ...)
#define ACK_DEBUG_PRINT_W(_format, ...)
#define ACK_DEBUG_PRINT_E(_format, ...)
#define ACK_DEBUG_PRINT_C(_format, ...)

#endif

#ifdef __cplusplus
}
#endif

#endif // def __ACK_DEBUG_PRINT_H__
