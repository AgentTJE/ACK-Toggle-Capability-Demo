/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_DASH_REPLENISHMENT_H__
#define __ACK_DASH_REPLENISHMENT_H__

#include "ack_user_config.h"
#include "ack_types.h"
#include "ack_generated_volume.h"
#include "ack_generated_weight.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ACK_DASH_REPLENISHMENT

// Indicates the kind of inventory level described by ACKInventoryLevel_t.
typedef enum _ACKInventoryLevelKind_t
{
    ACK_INVENTORY_LEVEL_NONE,
    ACK_INVENTORY_LEVEL_COUNT,
    ACK_INVENTORY_LEVEL_WEIGHT,
    ACK_INVENTORY_LEVEL_VOLUME,
    ACK_INVENTORY_LEVEL_PERCENTAGE
}
ACKInventoryLevelKind_t;

// Describes an inventory level. Used as input parameters to inventory-related routines.
typedef struct _ACKInventoryLevel_t
{
    ACKInventoryLevelKind_t Kind;

    union
    {
        uint32_t Count;
        
        struct
        {
            float Value;
            ack_weight_unit Unit;
        }
        Weight;

        struct
        {
            float Value;
            ack_volume_unit Unit;
        }
        Volume;

        uint32_t Percentage;
    }
    Level;
}
ACKInventoryLevel_t;

// Call this to add an inventory level property to an event.
ACKError_t ACK_AddInventoryLevelProperty(
    uint32_t instance,
    const ACKStateCommon_t* pCommon,
    const ACKInventoryLevel_t* pInventoryLevel);

// Informs the Dash Replenishment Service of inventory consumption, for an Inventory.Usage controller.
ACKError_t ACK_SendDashReplenishmentUsageConsumedEvent(
    uint32_t instance,
    const ACKInventoryLevel_t* pLevel,
    uint64_t timeOfSample);

#endif // def ACK_DASH_REPLENISHMENT

#ifdef __cplusplus
}
#endif

#endif // def __ACK_DASH_REPLENISHMENT_H__
