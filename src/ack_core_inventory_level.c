/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_inventory_level.h"
#include "ack_core_volume.h"
#include "ack_core_weight.h"
#include "ack_generated_inventory_count.h"
#include "ack_generated_inventory_level.h"
#include "ack_generated_percentage.h"

#ifdef ACK_DASH_REPLENISHMENT

bool InventoryLevelSave(const ACKDocContainer_t* pObject, unsigned member, const ACKInventoryLevel_t* pLevel)
{
    ACKDocContainer_t object;
    ACKDocContainer_t valueObject;
    int type;
    bool result;

    result = ACKDoc_AppendObject(pObject, member, &object)

        // Polymorphic object; members of the object selected by @type go into a 'value' subobject.
        && ACKDoc_AppendObject(&object, ack_inventory_level_object_member_value, &valueObject);

    if (result)
    {
        switch (pLevel->Kind)
        {
        case ACK_INVENTORY_LEVEL_COUNT:
            type = ack_inventory_level_object_type_count;
            result = ACKDoc_AppendInt64(&valueObject, ack_inventory_count_object_member_value, pLevel->Level.Count);
            break;

        case ACK_INVENTORY_LEVEL_PERCENTAGE:
            type = ack_inventory_level_object_type_percentage;
            result = ACKDoc_AppendInt64(&valueObject, ack_percentage_object_member_value, pLevel->Level.Percentage);
            break;

        case ACK_INVENTORY_LEVEL_WEIGHT:
            type = ack_inventory_level_object_type_weight;
            result = WeightSave(&valueObject, pLevel->Level.Weight.Unit, pLevel->Level.Weight.Value);
            break;

        case ACK_INVENTORY_LEVEL_VOLUME:
            type = ack_inventory_level_object_type_volume;
            result = VolumeSave(&valueObject, pLevel->Level.Volume.Unit, pLevel->Level.Volume.Value);
            break;

        default:
            result = false;
            break;
        }

        if (result)
        {
            result = ACKDoc_AppendInt(&object, ack_inventory_level_object_member_type, type);
        }
    }

    return result;
}

#endif // def ACK_DASH_REPLENISHMENT
