/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_DIRECTIVES_H__
#define __ACK_DIRECTIVES_H__

#include "ack.h"
#include "ack_core_heaplet.h"
#include "ack_user_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

ACKError_t ProcessAlexaDirective(const ACKDirectiveParams_t* pDocument);

#ifdef ACK_BRIGHTNESS_CONTROLLER
ACKError_t ProcessBrightnessControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_COLOR_CONTROLLER
ACKError_t ProcessColorControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_COLOR_TEMPERATURE_CONTROLLER
ACKError_t ProcessColorTemperatureControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_COOKING
ACKError_t ProcessCookingDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_COOKING_FOOD_TEMPERATURE_CONTROLLER
ACKError_t ProcessFoodTemperatureControllerCookingDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_COOKING_PRESET_CONTROLLER
ACKError_t ProcessCookingPresetControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_COOKING_TEMPERATURE_CONTROLLER
ACKError_t ProcessCookingTemperatureControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_COOKING_TIME_CONTROLLER
ACKError_t ProcessCookingTimeControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_DEVICE_USAGE
ACKError_t ProcessDeviceUsageDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_MODE_CONTROLLER
ACKError_t ProcessModeControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_POWER_CONTROLLER
ACKError_t ProcessPowerControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_PERCENTAGE_CONTROLLER
ACKError_t ProcessPercentageControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_THERMOSTAT_CONTROLLER
ACKError_t ProcessThermostatControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_RANGE_CONTROLLER
ACKError_t ProcessRangeControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_TIME_HOLD_CONTROLLER
ACKError_t ProcessTimeHoldControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

#ifdef ACK_TOGGLE_CONTROLLER
ACKError_t ProcessToggleControllerDirective(const ACKDirectiveParams_t* pDocument);
#endif

ACKError_t ProcessDomBinaryFormDirective(HeapletTag_t documentTag, HeapletTag_t directiveInfoTag);

#ifdef __cplusplus
}
#endif

#endif // def __ACK_DIRECTIVES_H__
