/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_core_common.h"
#include "ack_core_device_usage.h"
#include "ack_core_document_wrappers.h"
#include "ack_core_event_builder.h"
#include "ack_core_outbound_events.h"
#include "ack_device_usage.h"
#include "ack_user_config.h"
#include "ack_generated_ack.h"
#include "ack_generated_alexa_device_usage_estimation.h"
#include "ack_generated_alexa_device_usage_meter.h"
#include <inttypes.h>

#ifdef ACK_DEVICE_USAGE

// Forward declarations.
static bool MeterIntervalSave_(
    const ACKDocContainer_t* pTo,
    unsigned member,
    const ACKDeviceUsageMeterInterval_t* pInterval);

static bool MeterIntervalsArraySave_(
    const ACKDocContainer_t* pTo,
    unsigned member,
    const ACKDeviceUsageMeterInterval_t* pIntervals,
    unsigned intervalCount);

static bool ProcessReportMeasurementsDirective_(const ACKDirectiveParams_t* pDocument);
static bool ProcessReduceResolutionDirective_(const ACKDirectiveParams_t* pDocument);
static bool ProcessInvalidMeasurementErrorDirective_(const ACKDirectiveParams_t* pDocument);

// Calls user-supplied routine to process the directive.
ACKError_t ProcessDeviceUsageDirective(const ACKDirectiveParams_t* pDocument)
{
    bool result;

    switch (pDocument->Name)
    {
    case ack_alexa_device_usage_meter_directive_name_report_measurements:
        result = ProcessReportMeasurementsDirective_(pDocument);
        break;
    case ack_alexa_device_usage_meter_directive_name_reduce_resolution:
        result = ProcessReduceResolutionDirective_(pDocument);
        break;
    case ack_alexa_device_usage_meter_directive_name_invalid_measurement_error:
        result = ProcessInvalidMeasurementErrorDirective_(pDocument);
        break;
    default:
        return ACK_ERR_PROTOCOL;
    }

    return result ? ACK_NO_ERROR : ACK_ERR_PROTOCOL;
}

ACKError_t ACK_SendDeviceUsageMeterMeasurementsReportEvent(
#ifdef ACK_COMPOUND_DEVICE
    uint16_t endpointId,
#endif
    const ACKDeviceUsageMeterMeasurements_t* pMeasurements)
{
    ACKError_t retval;

    ACK_DEBUG_PRINT_I("Sending Alexa.DeviceUsage.Meter MeasurementsReport event");

    ACK_REENTRY_CHECK;

    OutboundEventReset();

#ifdef ACK_COMPOUND_DEVICE
    g_eventEndpointId = endpointId;
#endif

    if (!EventBuilderStart(
        0, // no correlation id
        ack_namespace_alexa_device_usage_meter,
        ack_alexa_device_usage_meter_event_name_measurements_report,
        0, // instance not used for this namespace
        EVENT_FLAGS_NONE)

        || !DeviceUsageMeasurementsReportObjectSave(&g_eventDocument.PayloadObject, pMeasurements))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    g_pendingOutboundEvent.Flags.Bits.EventPending = true;

    retval = OutboundEventSend();
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Error %u sending MeasurementsReport event.", retval);
        goto cleanup0;
    }

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    EventBuilderEnd();
    return retval;
}

// Helper to save a MeasurementsReportObject
bool DeviceUsageMeasurementsReportObjectSave(
    const ACKDocContainer_t* pTo,
    const ACKDeviceUsageMeterMeasurements_t* pMeasurements)
{
    // At least one set of measurements needs to have a value
    if ((pMeasurements->ElectricityIntervalCount == 0
        && pMeasurements->NaturalGasIntervalCount == 0)

        || (pMeasurements->ElectricityIntervalCount > 0
            && !MeterIntervalsArraySave_(
                pTo,
                ack_alexa_device_usage_meter_measurements_report_event_payload_object_member_electricity_intervals,
                pMeasurements->ElectricityIntervals,
                pMeasurements->ElectricityIntervalCount))

        || (pMeasurements->NaturalGasIntervalCount > 0
            && !MeterIntervalsArraySave_(
                pTo,
                ack_alexa_device_usage_meter_measurements_report_event_payload_object_member_natural_gas_intervals,
                pMeasurements->NaturalGasIntervals,
                pMeasurements->NaturalGasIntervalCount)))
    {
        return false;
    }

    return true;
}

// Private helpers to save objects and process directives
static bool MeterIntervalSave_(
    const ACKDocContainer_t* pTo,
    unsigned member,
    const ACKDeviceUsageMeterInterval_t* pInterval)
{
    ACKDocContainer_t intervalObject;

    return (pInterval != NULL)
        && ACKDoc_AppendObject(pTo, member, &intervalObject)
        && ACKDoc_AppendDouble(
            &intervalObject,
            ack_alexa_device_usage_meter_interval_object_member_usage,
            pInterval->Usage)
        && ACKDoc_AppendInt64(
            &intervalObject,
            ack_alexa_device_usage_meter_interval_object_member_start,
            pInterval->Start)
        && ACKDoc_AppendInt64(
            &intervalObject,
            ack_alexa_device_usage_meter_interval_object_member_end,
            pInterval->End);
}

static bool MeterIntervalsArraySave_(
    const ACKDocContainer_t* pTo,
    unsigned member,
    const ACKDeviceUsageMeterInterval_t* pIntervals,
    unsigned intervalCount)
{
    ACKDocContainer_t intervalsArray;
    unsigned index;

    if ((pIntervals == NULL && intervalCount > 0)
        || !ACKDoc_AppendArray(pTo, member, &intervalsArray))
    {
        return false;
    }

    for (index = 0; index < intervalCount; index++)
    {
        if (!MeterIntervalSave_(&intervalsArray, 0, pIntervals + index))
        {
            return false;
        }
    }

    return true;
}

static bool ProcessReportMeasurementsDirective_(const ACKDirectiveParams_t* pDocument)
{
    // ReportMeasurements directive has no payload

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnDeviceUsageMeterReportMeasurementsDirective(pDocument->CorrelationId);

    ACK_REENTRY_SHIELDS_DOWN;

    return true;
}

static bool ProcessReduceResolutionDirective_(const ACKDirectiveParams_t* pDocument)
{
    int32_t reduceResolutionLimit;
    int64_t reduceResolutionDurationMilliseconds;

    if (!ACKDoc_GetInt(
        &pDocument->PayloadObject,
        ack_alexa_device_usage_meter_reduce_resolution_payload_object_member_limit,
        &reduceResolutionLimit)
        || !ACKDoc_GetInt64(
            &pDocument->PayloadObject,
            ack_alexa_device_usage_meter_reduce_resolution_payload_object_member_duration,
            &reduceResolutionDurationMilliseconds))
    {
        return false;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnDeviceUsageMeterReduceResolutionDirective(
        pDocument->CorrelationId,
        (uint32_t)reduceResolutionLimit,
        (uint64_t)reduceResolutionDurationMilliseconds);

    ACK_REENTRY_SHIELDS_DOWN;

    return true;
}

static bool ProcessInvalidMeasurementErrorDirective_(const ACKDirectiveParams_t* pDocument)
{
    ACKDocContainer_t invalidMeasurementObject;
    ACKDeviceUsageMeterKind_t invalidMeasurementKind;
    int invalidMeasrementErrorCode;
    int64_t invalidMeasurementTimeOfError;

    if (ACKDoc_GetObject(
        &pDocument->PayloadObject,
        ack_alexa_device_usage_meter_invalid_measurement_error_payload_object_member_electricity,
        &invalidMeasurementObject))
    {
        invalidMeasurementKind = ACK_DEVICE_USAGE_METER_ELECTRICITY;
    }
    else if (ACKDoc_GetObject(
        &pDocument->PayloadObject,
        ack_alexa_device_usage_meter_invalid_measurement_error_payload_object_member_natural_gas,
        &invalidMeasurementObject))
    {
        invalidMeasurementKind = ACK_DEVICE_USAGE_METER_NATURAL_GAS;
    }
    else
    {
        return false;
    }

    if (!ACKDoc_GetInt(
        &invalidMeasurementObject,
        ack_alexa_device_usage_meter_invalid_measurement_error_object_member_error_code,
        &invalidMeasrementErrorCode)
        || !ACKDoc_GetInt64(
            &invalidMeasurementObject,
            ack_alexa_device_usage_meter_invalid_measurement_error_object_member_time_of_error,
            &invalidMeasurementTimeOfError))
    {
        return false;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnDeviceUsageMeterInvalidMeasurementErrorDirective(
        pDocument->CorrelationId,
        invalidMeasurementKind,
        (ack_alexa_device_usage_meter_invalid_measurement_error_code)invalidMeasrementErrorCode,
        (uint64_t)invalidMeasurementTimeOfError);

    ACK_REENTRY_SHIELDS_DOWN;

    return true;
}

#endif // def ACK_DEVICE_USAGE
