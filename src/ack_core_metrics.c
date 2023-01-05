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
#include "ack_core_heaplet.h"
#include "ack_metrics.h"
#include "ack_core_protobuf_utilities.h"
#include "ack_user_config.h"
#include "acp.pb.h"
#include "ack_user_module.h"

#ifdef ACK_METRICS

ACKError_t ACK_SendMetric(
    acp_cmd_record_dem_metric_Type type,
    size_t datapointCount,
    const ACKMetricDatapoint_t* pDatapoints)
{
    ACK_DEBUG_PRINT_I("Sending metric.");

    return ACKModule_SendMetric(type, datapointCount, pDatapoints);
}

ACKError_t ACK_SendDiscreteMetric(
    acp_cmd_record_dem_metric_Type metricType,
    const char* pDatapointName,
    const char* pDatapointValue)
{
    ACKMetricDatapoint_t datapoint;
    datapoint.Name = pDatapointName;
    datapoint.Type = acp_metric_datapoint_Type_DISCRETE;
    datapoint.Value.Discrete = pDatapointValue;

    return ACK_SendMetric(metricType, 1, &datapoint);
}

ACKError_t ACK_SendCounterMetric(
    acp_cmd_record_dem_metric_Type metricType,
    const char* pDatapointName,
    uint32_t datapointValue)
{
    ACKMetricDatapoint_t datapoint;
    datapoint.Name = pDatapointName;
    datapoint.Type = acp_metric_datapoint_Type_COUNTER;
    datapoint.Value.Counter = datapointValue;

    return ACK_SendMetric(metricType, 1, &datapoint);
}

ACKError_t ACK_SendNumericValueMetric(
    acp_cmd_record_dem_metric_Type metricType,
    const char* pDatapointName,
    float datapointValue)
{
    ACKMetricDatapoint_t datapoint;
    datapoint.Name = pDatapointName;
    datapoint.Type = acp_metric_datapoint_Type_GAUGE;
    datapoint.Value.Gauge = datapointValue;

    return ACK_SendMetric(metricType, 1, &datapoint);
}

ACKError_t ACK_SendNumericValueChangedMetric(
    acp_cmd_record_dem_metric_Type metricType,
    const char* pOldDatapointName,
    float oldValue,
    const char* pNewDatapointName,
    float newValue)
{
    ACKMetricDatapoint_t datapoint[2];
    datapoint[0].Name = pOldDatapointName;
    datapoint[0].Type = acp_metric_datapoint_Type_GAUGE;
    datapoint[0].Value.Gauge = oldValue;

    datapoint[1].Name = pNewDatapointName;
    datapoint[1].Type = acp_metric_datapoint_Type_GAUGE;
    datapoint[1].Value.Gauge = newValue;

    return ACK_SendMetric(metricType, 2, datapoint);
}

ACKError_t ACK_SendErrorMetricWithNumericValue(const char* pDataName, float value)
{
    return ACK_SendNumericValueMetric(acp_cmd_record_dem_metric_Type_DEVICE_ERROR, pDataName, value);
}

ACKError_t ACK_SendErrorMetricWithStringValue(const char* pDataName, const char* pText)
{
    return ACK_SendDiscreteMetric(acp_cmd_record_dem_metric_Type_DEVICE_ERROR, pDataName, pText);
}

ACKError_t ACK_SendErrorMetric(const char* pDataName)
{
    return ACK_SendDiscreteMetric(acp_cmd_record_dem_metric_Type_DEVICE_ERROR, pDataName, NULL);
}

ACKError_t ACK_SendUserPresentMetricWithStringValue(const char* pDataName, const char* pText)
{
    return ACK_SendDiscreteMetric(acp_cmd_record_dem_metric_Type_USER_PRESENT, pDataName, pText);
}

ACKError_t ACK_SendUserPresentMetric(const char* pDataName)
{
    return ACK_SendDiscreteMetric(acp_cmd_record_dem_metric_Type_USER_PRESENT, pDataName, NULL);
}

ACKError_t ACK_SendUsageReportMetricWithStringValue(const char* pDataName, const char* pText)
{
    return ACK_SendDiscreteMetric(acp_cmd_record_dem_metric_Type_USAGE_REPORT, pDataName, pText);
}

ACKError_t ACK_SendUsageReportMetricWithNumericValue(const char* pDataName, float value)
{
    return ACK_SendNumericValueMetric(acp_cmd_record_dem_metric_Type_USAGE_REPORT, pDataName, value);
}

ACKError_t ACK_SendUsageReportMetric(const char* pDataName)
{
    return ACK_SendDiscreteMetric(acp_cmd_record_dem_metric_Type_USAGE_REPORT, pDataName, NULL);
}

ACKError_t ACK_SendUsageReportIncrementMetric(const char* pDataName)
{
    return ACK_SendCounterMetric(acp_cmd_record_dem_metric_Type_USAGE_REPORT, pDataName, 1);
}

ACKError_t ACK_SendUsageReportCountMetric(const char* pDataName, uint32_t count)
{
    return ACK_SendCounterMetric(acp_cmd_record_dem_metric_Type_USAGE_REPORT, pDataName, count);
}

#endif // def ACK_METRICS
