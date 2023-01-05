/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_core_common.h"
#include "ack_core_document_wrappers.h"
#include "ack_core_event_builder.h"
#include "ack_core_inventory_level.h"
#include "ack_core_outbound_events.h"
#include "ack_dash_replenishment.h"
#include "ack_user_config.h"
#include "ack_generated_ack.h"
#include "ack_generated_inventory_level.h"
#include "ack_generated_alexa_inventory_level_sensor.h"
#include "ack_generated_alexa_inventory_usage_sensor.h"
#include <inttypes.h>

#ifdef ACK_DASH_REPLENISHMENT

static ACKError_t SendEvent_(void);

ACKError_t ACK_AddInventoryLevelProperty(
    uint32_t instance,
    const ACKStateCommon_t* pCommon,
    const ACKInventoryLevel_t* pInventoryLevel)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_inventory_level_sensor,
        &instance,
        ack_alexa_inventory_level_sensor_property_name_level,
        &propertyObject)

        && InventoryLevelSave(&propertyObject, ack_property_object_member_value, pInventoryLevel);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

ACKError_t ACK_SendDashReplenishmentUsageConsumedEvent(
    uint32_t instance,
    const ACKInventoryLevel_t* pLevel,
    uint64_t timeOfSample)
{
    ACKError_t retval;

    ACK_DEBUG_PRINT_I("Sending Alexa.InventoryUsageSensor consumed event, instance %"PRIu32".", instance);

    ACK_REENTRY_CHECK;

    OutboundEventReset();

    // TODO: endpoint id handling

    if (!EventBuilderStart(
        0, // no correlation id
        ack_namespace_alexa_inventory_usage_sensor,
        ack_alexa_inventory_usage_sensor_event_name_inventory_consumed,
        instance,
        EVENT_FLAGS_NONE)

        || !InventoryLevelSave(
            &g_eventDocument.PayloadObject,
            ack_alexa_inventory_usage_sensor_inventory_consumed_event_payload_object_member_usage,
            pLevel)

        || !ACKDoc_AppendInt64(
            &g_eventDocument.PayloadObject,
            ack_alexa_inventory_usage_sensor_inventory_consumed_event_payload_object_member_time_of_sample,
            (int64_t)timeOfSample))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    retval = SendEvent_();
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    EventBuilderEnd();
    return retval;
}

static ACKError_t SendEvent_(void)
{
    ACKError_t retval;

    g_pendingOutboundEvent.Flags.Bits.EventPending = true;

    retval = OutboundEventSend();
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Error %u sending inventory event.", retval);
    }

    return retval;
}

#endif // def ACK_DASH_REPLENISHMENT
