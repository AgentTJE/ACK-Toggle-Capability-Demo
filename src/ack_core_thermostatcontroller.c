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
#include "ack_core_directives.h"
#include "ack_core_document_wrappers.h"
#include "ack_core_temperature.h"
#include "ack_core_time_interval.h"
#include "ack_thermostat_controller.h"
#include "ack_user_config.h"
#include "ack_generated_alexa_thermostat_controller.h"

#ifdef ACK_THERMOSTAT_CONTROLLER

// Forward declarations.
static bool SetTargetTemperature_(const ACKDirectiveParams_t* pDocument);
static bool AdjustTargetTemperature_(const ACKDirectiveParams_t* pDocument);
static bool SetMode_(const ACKDirectiveParams_t* pDocument);
static bool ResumeSchedule_(const ACKDirectiveParams_t* pDocument);

static ACKError_t AddSetpointProperty_(
    const ACKStateCommon_t* pCommon,
    unsigned member,
    const ACKTemperature_t* pSetpoint);

// Calls user-supplied routine to process the directive.
ACKError_t ProcessThermostatControllerDirective(const ACKDirectiveParams_t* pDocument)
{
    bool result;

    switch (pDocument->Name)
    {
    case ack_alexa_thermostat_controller_directive_name_set_target_temperature:
        result = SetTargetTemperature_(pDocument);
        break;

    case ack_alexa_thermostat_controller_directive_name_adjust_target_temperature:
        result = AdjustTargetTemperature_(pDocument);
        break;

    case ack_alexa_thermostat_controller_directive_name_set_thermostat_mode:
        result = SetMode_(pDocument);
        break;

    case ack_alexa_thermostat_controller_directive_name_resume_schedule:
        result = ResumeSchedule_(pDocument);
        break;

    default:
        result = false;
        break;
    }

    return result ? ACK_NO_ERROR : ACK_ERR_PROTOCOL;
}

static bool SetTargetTemperature_(const ACKDirectiveParams_t* pDocument)
{
    ACKThermostatControllerSetTargetTemperatureParams_t params = {{0}};

    (void)TemperatureLoad(
        &pDocument->PayloadObject,
        ack_alexa_thermostat_controller_set_target_temperature_payload_object_member_target_setpoint,
        &params.TargetSetpointTemperature.TemperatureScale,
        &params.TargetSetpointTemperature.Value);

    (void)TemperatureLoad(
        &pDocument->PayloadObject,
        ack_alexa_thermostat_controller_set_target_temperature_payload_object_member_lower_setpoint,
        &params.LowerSetpointTemperature.TemperatureScale,
        &params.LowerSetpointTemperature.Value);

    (void)TemperatureLoad(
        &pDocument->PayloadObject,
        ack_alexa_thermostat_controller_set_target_temperature_payload_object_member_upper_setpoint,
        &params.UpperSetpointTemperature.TemperatureScale,
        &params.UpperSetpointTemperature.Value);

    (void)TimeIntervalLoad(
        &pDocument->PayloadObject,
        ack_alexa_thermostat_controller_set_target_temperature_payload_object_member_schedule,
        &params.Schedule.StartTime,
        &params.Schedule.EndTime,
        &params.Schedule.DurationMilliseconds);

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnThermostatControllerSetTargetTemperatureDirective(pDocument->CorrelationId, &params);

    ACK_REENTRY_SHIELDS_DOWN;

    return true;
}

static bool AdjustTargetTemperature_(const ACKDirectiveParams_t* pDocument)
{
    ACKTemperature_t temperatureDelta;

    if (!TemperatureLoad(
        &pDocument->PayloadObject,
        ack_alexa_thermostat_controller_adjust_target_temperature_payload_object_member_target_setpoint_delta,
        &temperatureDelta.TemperatureScale,
        &temperatureDelta.Value))
    {
        return false;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnThermostatControllerAdjustTargetTemperatureDirective(
        pDocument->CorrelationId,
        &temperatureDelta);

    ACK_REENTRY_SHIELDS_DOWN;

    return true;
}

static bool SetMode_(const ACKDirectiveParams_t* pDocument)
{
    ACKDocContainer_t modeObject;
    int mode;

    if (!ACKDoc_GetObject(
        &pDocument->PayloadObject,
        ack_alexa_thermostat_controller_set_mode_payload_object_member_thermostat_mode,
        &modeObject)

        || !ACKDoc_GetInt(&modeObject, ack_thermostat_mode_object_member_value, &mode))
    {
        return false;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnThermostatControllerSetThermostatModeDirective(pDocument->CorrelationId, mode);

    ACK_REENTRY_SHIELDS_DOWN;

    return true;
}

static bool ResumeSchedule_(const ACKDirectiveParams_t* pDocument)
{
    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnThermostatControllerResumeScheduleDirective(pDocument->CorrelationId);

    ACK_REENTRY_SHIELDS_DOWN;

    return true;
}

ACKError_t ACK_AddThermostatControllerTargetSetpointProperty(
    const ACKStateCommon_t* pCommon,
    const ACKTemperature_t* pTargetSetpoint)
{
    ACK_REENTRY_CHECK;

    return AddSetpointProperty_(
        pCommon,
        ack_alexa_thermostat_controller_property_name_target_setpoint,
        pTargetSetpoint);
}

ACKError_t ACK_AddThermostatControllerLowerSetpointProperty(
    const ACKStateCommon_t* pCommon,
    const ACKTemperature_t* pLowerSetpoint)
{
    ACK_REENTRY_CHECK;

    return AddSetpointProperty_(
        pCommon,
        ack_alexa_thermostat_controller_property_name_lower_setpoint,
        pLowerSetpoint);
}

ACKError_t ACK_AddThermostatControllerUpperSetpointProperty(
    const ACKStateCommon_t* pCommon,
    const ACKTemperature_t* pUpperSetpoint)
{
    ACK_REENTRY_CHECK;

    return AddSetpointProperty_(
        pCommon,
        ack_alexa_thermostat_controller_property_name_upper_setpoint,
        pUpperSetpoint);
}

static ACKError_t AddSetpointProperty_(
    const ACKStateCommon_t* pCommon,
    unsigned member,
    const ACKTemperature_t* pSetpoint)
{
    ACKDocContainer_t propertyObject;
    bool result;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_thermostat_controller,
        NULL, // No instance for this capability
        member,
        &propertyObject)

        && TemperatureSave(
            &propertyObject,
            ack_property_object_member_value,
            pSetpoint->TemperatureScale,
            pSetpoint->Value);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

ACKError_t ACK_AddThermostatControllerModeProperty(
    const ACKStateCommon_t* pCommon,
    ack_thermostat_mode thermostatMode)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_thermostat_controller,
        NULL, // No instance for this capability
        ack_alexa_thermostat_controller_property_name_thermostat_mode,
        &propertyObject)

        && ACKDoc_AppendInt(&propertyObject, ack_property_object_member_value, thermostatMode);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

ACKError_t ACK_AddThermostatControllerAdaptiveRecoveryStatusProperty(
    const ACKStateCommon_t* pCommon,
    ack_alexa_thermostat_controller_adaptive_recovery_status adaptiveRecoveryStatus)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_thermostat_controller,
        NULL, // No instance for this capability
        ack_alexa_thermostat_controller_property_name_adaptive_recovery_status,
        &propertyObject)

        && ACKDoc_AppendInt(
            &propertyObject,
            ack_property_object_member_value,
            adaptiveRecoveryStatus);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#endif // def ACK_THERMOSTAT_CONTROLLER
