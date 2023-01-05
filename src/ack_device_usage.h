/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_DEVICE_USAGE_H__
#define __ACK_DEVICE_USAGE_H__

#include "ack_user_config.h"
#include "ack_types.h"
#include "ack_generated_alexa_device_usage_estimation.h"
#include "ack_generated_alexa_device_usage_meter.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ACK_DEVICE_USAGE

typedef struct _ACKDeviceUsageEstimationQuantity_t
{
    double Value;
    ack_alexa_device_usage_estimation_units Units;
}
ACKDeviceUsageEstimationQuantity_t;

// Indicates the kind of reading described.
typedef enum _ACKDeviceUsageMeterKind_t
{
    ACK_DEVICE_USAGE_METER_ELECTRICITY,
    ACK_DEVICE_USAGE_METER_NATURAL_GAS
}
ACKDeviceUsageMeterKind_t;

typedef struct _ACKDeviceUsageMeterInterval_t
{
    double Usage;
    uint64_t Start; // Posix epoch value in milliseconds.
    uint64_t End;   // Posix epoch value in milliseconds.
}
ACKDeviceUsageMeterInterval_t;

typedef struct _ACKDeviceUsageMeterMeasurements_t
{
    ACKDeviceUsageMeterInterval_t* ElectricityIntervals;
    unsigned ElectricityIntervalCount;

    ACKDeviceUsageMeterInterval_t* NaturalGasIntervals;
    unsigned NaturalGasIntervalCount;
}
ACKDeviceUsageMeterMeasurements_t;

// You must implement these routines.
void ACKUser_OnDeviceUsageMeterReportMeasurementsDirective(int32_t correlationId);

void ACKUser_OnDeviceUsageMeterReduceResolutionDirective(
    int32_t correlationId,
    uint32_t limit,
    uint64_t durationMilliseconds);

void ACKUser_OnDeviceUsageMeterInvalidMeasurementErrorDirective(
    int32_t correlationId,
    ACKDeviceUsageMeterKind_t kind,
    ack_alexa_device_usage_meter_invalid_measurement_error_code errorCode,
    uint64_t timeOfError);

// Call this to add a property representing a measurement report to an outbound event.
// At least one measurement property should have a count greater than 0.
#ifdef ACK_COMPOUND_DEVICE
ACKError_t ACK_SendDeviceUsageMeterMeasurementsReportEvent(
    uint16_t endpointId,
    const ACKDeviceUsageMeterMeasurements_t* pMeasurements);
#else
ACKError_t ACK_SendDeviceUsageMeterMeasurementsReportEvent(
    const ACKDeviceUsageMeterMeasurements_t* pMeasurements);
#endif

#endif // def ACK_DEVICE_USAGE

#ifdef __cplusplus
}
#endif

#endif // def __ACK_DEVICE_USAGE_H__
