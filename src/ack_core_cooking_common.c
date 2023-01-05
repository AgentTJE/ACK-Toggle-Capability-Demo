/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_cooking_common.h"
#include "ack_core_cooking_mode.h"
#include "ack_core_food_doneness.h"
#include "ack_core_food_item.h"
#include "ack_core_power_level.h"
#include "ack_core_temperature.h"
#include "ack_user_config.h"
#include "ack_generated_ack.h"

#ifdef ACK_COOKING

bool CookingExtractDirectiveParams(
    const ACKDirectiveParams_t* pDocument,
    unsigned fields,
    const CookingDirectiveMembers* pFieldMembers,
    ACKCookingDirectiveParams_t* pParams)
{
    bool result;

    memset(pParams, 0, sizeof(*pParams));

    // Cooking mode.
    if (fields & CDFP_MODE_PRESENT)
    {
        result = CookingModeLoad(
            &pDocument->PayloadObject,
            pFieldMembers->Mode,
            &pParams->CookingMode,
            &pParams->CustomModeName);
        if (result)
        {
            pParams->Flags |= ACK_COOKING_DIRECTIVE_COOKING_MODE_VALID;
        }
        else if (fields & CDFP_MODE_REQUIRED)
        {
            return false;
        }
    }

    // Food item.
    if (fields & CDFP_FOODITEM_PRESENT)
    {
        result = FoodItemLoad(
            &pDocument->PayloadObject,
            pFieldMembers->FoodItem,
            &pParams->FoodName,
            &pParams->FoodQuantity);
        if (result)
        {
            pParams->Flags |= ACK_COOKING_DIRECTIVE_FOOD_QUANTITY_VALID;
        }
#ifdef CDFP_FOODITEM_REQUIRED
        else if (fields & CDFP_FOODITEM_REQUIRED)
        {
            return false;
        }
#endif
    }

    // Temperature. This can be e.g. food temperature or appliance temperature.
    if (fields & CDFP_TEMPERATURE_PRESENT)
    {
        result = TemperatureLoad(
            &pDocument->PayloadObject,
            pFieldMembers->Temperature,
            &pParams->TemperatureScale,
            &pParams->Temperature);
        if (result)
        {
            pParams->Flags |= ACK_COOKING_DIRECTIVE_TEMPERATURE_VALID;
        }
        else if (fields & CDFP_TEMPERATURE_REQUIRED)
        {
            return false;
        }
    }

    // Preset.
#if !defined(CDFP_PRESET_PRESENT) && !defined(CDFP_PRESET_REQUIRED)
    if ((fields & CDFP_PRESET_PRESENT_AND_REQUIRED)
        && !ACKDoc_GetString(&pDocument->PayloadObject, pFieldMembers->Preset, &pParams->PresetName))
    {
        return false;
    }
#else
    if ((fields & CDFP_PRESET_PRESENT)
        && !ACKDoc_GetString(&pDocument->PayloadObject, pFieldMembers->Preset, &pParams->PresetName))
    {
        if (fields & CDFP_PRESET_REQUIRED)
        {
            return false;
        }

        pParams->PresetName = NULL;            
    }
#endif

    // Duration.
#if !defined(CDFP_DURATION_PRESENT) && !defined(CDFP_DURATION_REQUIRED)
    if (fields & CDFP_DURATION_PRESENT_AND_REQUIRED)
    {
        ACK_STATIC_ASSERT(sizeof(pParams->Duration) == sizeof(int64_t));

        result = ACKDoc_GetInt64(
            &pDocument->PayloadObject,
            pFieldMembers->Duration,
            (int64_t*)&pParams->Duration);
        if (!result)
        {
            return false;
        }

        pParams->Flags |= ACK_COOKING_DIRECTIVE_DURATION_VALID;
    }
#else    
    if (fields & CDFP_DURATION_PRESENT)
    {
        ACK_STATIC_ASSERT(sizeof(pParams->Duration) == sizeof(int64_t));

        result = ACKDoc_GetInt64(
            &pDocument->PayloadObject,
            pFieldMembers->Duration,
            (int64_t*)&pParams->Duration);
        if (result)
        {
            pParams->Flags |= ACK_COOKING_DIRECTIVE_DURATION_VALID;
        }
        else if (fields & CDFP_DURATION_REQUIRED)
        {
            return false;
        }
    }
#endif

    // Power level.
    if (fields & CDFP_POWERLEVEL_PRESENT)
    {
        result = PowerLevelLoad(
            &pDocument->PayloadObject,
            pFieldMembers->PowerLevel,
            &pParams->PowerLevelKind,
            &pParams->PowerLevel);
        if (result)
        {
            pParams->Flags |= ACK_COOKING_DIRECTIVE_POWER_LEVEL_VALID;
        }
#ifdef CDFP_POWERLEVEL_REQUIRED
        else if (fields & CDFP_POWERLEVEL_REQUIRED)
        {
            return false;
        }
#endif
    }

    //FoodDoneness
    if (fields & CDFP_FOODDONENESS_PRESENT)
    {
        result = FoodDonenessLoad(
            &pDocument->PayloadObject,
            &pParams->FoodDoneness);

        if (result)
        {
            pParams->Flags |= ACK_COOKING_DIRECTIVE_FOOD_DONENESS_VALID;
        }
#ifdef CDFP_FOODDONENESS_REQUIRED
        else if (fields & CDFP_POWERLEVEL_REQUIRED)
        {
            return false;
        }
#endif
    }

    return true;
}

#endif // def ACK_COOKING
