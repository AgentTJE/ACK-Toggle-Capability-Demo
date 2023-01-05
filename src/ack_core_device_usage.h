#ifndef __ACK_CORE_DEVICE_USAGE_H__
#define __ACK_CORE_DEVICE_USAGE_H__

#include "ack.h"
#include "ack_device_usage.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ACK_DEVICE_USAGE

bool DeviceUsageMeasurementsReportObjectSave(
    const ACKDocContainer_t* pTo,
    const ACKDeviceUsageMeterMeasurements_t* pMeasurements);

#endif // def ACK_DEVICE_USAGE

#ifdef __cplusplus
}
#endif

#endif // ndef __ACK_CORE_DEVICE_USAGE_H__