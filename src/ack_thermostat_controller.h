/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_THERMOSTAT_CONTROLLER_H__
#define __ACK_THERMOSTAT_CONTROLLER_H__

#include "ack_user_config.h"
#include "ack_types.h"
#include "ack_generated_alexa_thermostat_controller.h"
#include "ack_generated_thermostat_mode.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ACK_THERMOSTAT_CONTROLLER

typedef struct _ACKThermostatControllerSetTargetTemperatureParams_t
{
    ACKTemperature_t TargetSetpointTemperature;
    ACKTemperature_t LowerSetpointTemperature;
    ACKTemperature_t UpperSetpointTemperature;
    ACKTimeInterval_t Schedule;
}
ACKThermostatControllerSetTargetTemperatureParams_t;

// You must implement these routines.
void ACKUser_OnThermostatControllerSetTargetTemperatureDirective(
    int32_t correlationId,
    const ACKThermostatControllerSetTargetTemperatureParams_t* pSetTargetTemperature);

void ACKUser_OnThermostatControllerAdjustTargetTemperatureDirective(
    int32_t correlationId,
    const ACKTemperature_t* pTemperature);

void ACKUser_OnThermostatControllerSetThermostatModeDirective(
    int32_t correlationId,
    ack_thermostat_mode thermostatMode);

void ACKUser_OnThermostatControllerResumeScheduleDirective(int32_t correlationId);

// Call this to add a property representing the state of a thermostat controller to an outbound event.
ACKError_t ACK_AddThermostatControllerTargetSetpointProperty(
    const ACKStateCommon_t* pCommon,
    const ACKTemperature_t* pTargetSetpoint);

ACKError_t ACK_AddThermostatControllerLowerSetpointProperty(
    const ACKStateCommon_t* pCommon,
    const ACKTemperature_t* pLowerSetpoint);

ACKError_t ACK_AddThermostatControllerUpperSetpointProperty(
    const ACKStateCommon_t* pCommon,
    const ACKTemperature_t* pUpperSetpoint);

ACKError_t ACK_AddThermostatControllerModeProperty(
    const ACKStateCommon_t* pCommon,
    ack_thermostat_mode thermostatMode);

ACKError_t ACK_AddThermostatControllerAdaptiveRecoveryStatusProperty(
    const ACKStateCommon_t* pCommon,
    ack_alexa_thermostat_controller_adaptive_recovery_status adaptiveRecoveryStatus);

#endif // def ACK_THERMOSTAT_CONTROLLER

#ifdef __cplusplus
}
#endif

#endif // def __ACK_THERMOSTAT_CONTROLLER_H__
