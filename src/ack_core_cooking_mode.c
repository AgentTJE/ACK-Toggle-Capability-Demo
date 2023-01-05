/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_cooking_mode.h"
#include "ack_generated_ack.h"
#include "ack_generated_cooking_mode.h"

bool CookingModeLoad(
    const ACKDocContainer_t* pFrom,
    unsigned member,
    ack_cooking_mode* pMode,
    const char** ppCustomModeName)
{
    ACKDocContainer_t cookingModeObject;
    int cookingMode;
    const char* pCookingModeName;

    *ppCustomModeName = NULL;
    *pMode = (ack_cooking_mode)0;

    if (!ACKDoc_GetObject(pFrom, member, &cookingModeObject)
        || !ACKDoc_GetInt(&cookingModeObject, ack_cooking_mode_object_member_value, &cookingMode))
    {
        return false;
    }

    if (!ACKDoc_GetString(&cookingModeObject, ack_cooking_mode_object_member_custom_name, &pCookingModeName))
    {
        pCookingModeName = NULL;
    }

    *pMode = (ack_cooking_mode)cookingMode;
    *ppCustomModeName = pCookingModeName;

    return true;
}

bool CookingModeSave(
    const ACKDocContainer_t* pTo,
    unsigned member,
    ack_cooking_mode mode,
    const char* pCustomModeName)
{
    ACKDocContainer_t cookingModeObject;

    if (!ACKDoc_AppendObject(pTo, member, &cookingModeObject)
        || !ACKDoc_AppendInt(&cookingModeObject, ack_cooking_mode_object_member_value, mode)
        || (pCustomModeName 
            && !ACKDoc_AppendString(
                &cookingModeObject,
                ack_cooking_mode_object_member_custom_name,
                pCustomModeName)))
    {
        return false;
    }

    return true;
}
