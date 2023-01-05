/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_CORE_COOKING_COMMON_H__
#define __ACK_CORE_COOKING_COMMON_H__

#include "ack_cooking.h"
#include "ack_core_directives.h"
#include "ack_user_config.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ACK_COOKING

// Common cooking handling to extract parameters from any cooking directive.

// Field presence/requirement.
// Each field has 2 bits: one indicating presence, and another indicating required.
// If neither bit is set for a field, it is not loaded from the payload.
// If only the presence bit is set, the field is optional and is loaded if present.
// If both bits are set, the field must be present.
// (The 4th possible combo is that only the required bit is set for a field, which is not a supported state.)

// Mode: can be either optional or required.
#define CDFP_MODE_PRESENT (1 << 0)
#define CDFP_MODE_REQUIRED (1 << 1)

// Food item: currently, if present, always optional.
#define CDFP_FOODITEM_PRESENT (1 << 2)
//#define CDFP_FOODITEM_REQUIRED (1 << 3)

// Temperature: can be either optional or required.
#define CDFP_TEMPERATURE_PRESENT (1 << 4)
#define CDFP_TEMPERATURE_REQUIRED (1 << 5)

// Preset: currently, if present, always required.
#define CDFP_PRESET_PRESENT_AND_REQUIRED (1 << 6)
//#define CDFP_PRESET_PRESENT (1 << 6)
//#define CDFP_PRESET_REQUIRED (1 << 7)

// Duration: currently, if present, always required.
#define CDFP_DURATION_PRESENT_AND_REQUIRED (1 << 8)
//#define CDFP_DURATION_PRESENT (1 << 8)
//#define CDFP_DURATION_REQUIRED (1 << 9)

// Power level: currently, if present, always optional.
#define CDFP_POWERLEVEL_PRESENT (1 << 10)
//#define CDFP_POWERLEVEL_REQUIRED (1 << 11)

// FoodDoneness: currently, if present, always optional.
#define CDFP_FOODDONENESS_PRESENT (1 << 12)
//#define CDFP_FOODDONENESS_REQUIRED (1 << 13)

// Payload member names.
// Assumes that all enumerated member names can fit in 8 bits, which shouldn't be a problem
// because there are a small number of fields in each directive payload.
// This saves space on the stack.
typedef struct _CookingDirectiveMembers
{
    uint8_t Mode;
    uint8_t FoodItem;
    uint8_t Temperature;
    uint8_t Preset;
    uint8_t Duration;
    uint8_t PowerLevel;
    ack_food_doneness FoodDoneness;
}
CookingDirectiveMembers;

bool CookingExtractDirectiveParams(
    const ACKDirectiveParams_t* pDocument,
    unsigned fields,
    const CookingDirectiveMembers* pFieldMembers,
    ACKCookingDirectiveParams_t* pParams);

#endif // ACK_COOKING

#ifdef __cplusplus
}
#endif

#endif // ndef __ACK_CORE_COOKING_COMMON_H__
