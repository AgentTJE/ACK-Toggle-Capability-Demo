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
#include "ack_core_temperature.h"
#include "ack_cooking.h"
#include "ack_generated_ack.h"
#include "ack_generated_alexa_cooking_temperature_sensor.h"

#ifdef ACK_COOKING_TEMPERATURE_SENSOR

ACKError_t ACK_AddCookingTemperatureProperty(
    const ACKStateCommon_t* pCommon,
    ack_temperature_scale scale,
    float temperature)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
            pCommon,
            ack_namespace_alexa_cooking_temperature_sensor,
            NULL, // No instance for this capability
            ack_alexa_cooking_temperature_sensor_property_name_cooking_temperature,
            &propertyObject)
        && TemperatureSave(&propertyObject, ack_property_object_member_value, scale, temperature);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // def ACK_COOKING_TEMPERATURE_SENSOR
