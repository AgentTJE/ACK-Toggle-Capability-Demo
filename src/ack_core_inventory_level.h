/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_CORE_INVENTORY_LEVEL_H__
#define __ACK_CORE_INVENTORY_LEVEL_H__

#include "ack_dash_replenishment.h"
#include "ack_user_config.h"
#include "ack_document.h"
#include <stdbool.h>

#ifdef ACK_DASH_REPLENISHMENT

#ifdef __cplusplus
extern "C"
{
#endif

bool InventoryLevelSave(const ACKDocContainer_t* pObject, unsigned member, const ACKInventoryLevel_t* pLevel);

#ifdef __cplusplus
}
#endif

#endif // def ACK_DASH_REPLENISHMENT

#endif // ndef __ACK_CORE_INVENTORY_LEVEL_H__
