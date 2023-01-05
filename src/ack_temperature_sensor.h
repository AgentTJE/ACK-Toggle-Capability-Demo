/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_TEMPERATURE_SENSOR_H__
#define __ACK_TEMPERATURE_SENSOR_H__

#include "ack_user_config.h"
#include "ack_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ACK_TEMPERATURE_SENSOR

// Call this to add a property representing the state of a temperature sensor to an outbound event.
ACKError_t ACK_AddTemperatureProperty(
    const ACKStateCommon_t* pCommon, 
    ack_temperature_scale scale, 
    float temperature);

#endif // def ACK_TEMPERATURE_SENSOR

#ifdef __cplusplus
}
#endif

#endif // def __ACK_TEMPERATURE_SENSOR_H__
