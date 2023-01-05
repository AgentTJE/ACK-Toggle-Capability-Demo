/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_count.h"
#include "ack_core_item_quantity.h"
#include "ack_core_weight.h"
#include "ack_core_volume.h"
#include "ack_document.h"
#include "ack_generated_item_quantity.h"

bool ItemQuantityLoad(const ACKDocContainer_t* pFrom, unsigned member, ACKItemQuantity_t* pItemQuantity)
{
    ACKDocContainer_t object;
    ACKDocContainer_t valueObject;
    int itemQuantityType = 0;
    bool result;
    
    result = ACKDoc_GetObject(pFrom, member, &object)
        && ACKDoc_GetInt(&object, ack_item_quantity_object_member_type, &itemQuantityType)

        // Polymorphic object; members of the object selected by @type are in a 'value' subobject.
        && ACKDoc_GetObject(&object, ack_item_quantity_object_member_value, &valueObject);

    if (result)
    {
        switch (itemQuantityType)
        {
        case ack_item_quantity_object_type_count:
            result = CountLoad(&valueObject, &pItemQuantity->Unit.Size, &pItemQuantity->Quantity);
            break;

        case ack_item_quantity_object_type_volume:
            result = VolumeLoad(&valueObject, &pItemQuantity->Unit.VolumeUnit, &pItemQuantity->Quantity);
            break;

        case ack_item_quantity_object_type_weight:
            result = WeightLoad(&valueObject, &pItemQuantity->Unit.WeightUnit, &pItemQuantity->Quantity);
            break;

        default:
            result = false;
            break;
        }

        if (result)
        {
            pItemQuantity->UnitType = (ack_item_quantity_object_type)itemQuantityType;
        }
    }

    return result;
}

bool ItemQuantitySave(const ACKDocContainer_t* pTo, unsigned member, const ACKItemQuantity_t* pItemQuantity)
{
    ACKDocContainer_t itemQuantityObject;
    ACKDocContainer_t valueObject;
    ack_item_quantity_object_type type = (ack_item_quantity_object_type)0;
    bool result;

    result = ACKDoc_AppendObject(pTo, member, &itemQuantityObject)

        // Polymorphic object; members of the object selected by @type go into a 'value' subobject.
        && ACKDoc_AppendObject(&itemQuantityObject, ack_item_quantity_object_member_value, &valueObject);

    if (result)
    {
        switch (pItemQuantity->UnitType)
        {
        case ack_item_quantity_object_type_count:
            type = ack_item_quantity_object_type_count;
            result = CountSave(&valueObject, pItemQuantity->Unit.Size, pItemQuantity->Quantity);
            break;

        case ack_item_quantity_object_type_volume:
            type = ack_item_quantity_object_type_volume;
            result = VolumeSave(&valueObject, pItemQuantity->Unit.VolumeUnit, pItemQuantity->Quantity);
            break;

        case ack_item_quantity_object_type_weight:
            type = ack_item_quantity_object_type_weight;
            result = WeightSave(&valueObject, pItemQuantity->Unit.WeightUnit, pItemQuantity->Quantity);
            break;

        default:
            result = false;
            break;
        }

        if (result)
        {
            result = ACKDoc_AppendInt(&itemQuantityObject, ack_item_quantity_object_member_type, type);
        }
    }

    return result;
}
