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
#include "ack_core_directives.h"
#include "ack_core_document.h"
#include "ack_core_document_wrappers.h"
#include "ack_core_endpoint_id.h"
#include "ack_core_event_builder.h"
#include "ack_core_heaplet.h"
#include "ack_generated_ack.h"
#include "ack_generated_alexa.h"
#include <stdint.h>

ACKEventDocument_t g_eventDocument;
#ifdef ACK_COMPOUND_DEVICE
uint16_t g_eventEndpointId = ACK_COMPOUND_DEVICE_DEFAULT_ENDPOINT;
#endif

ACKError_t ProcessDomBinaryFormDirective(HeapletTag_t documentTag, HeapletTag_t directiveInfoTag)
{
    ACKDirectiveParams_t *pDirectiveInfo;
    ACKDocContainer_t directiveObject;
    ACKDocContainer_t tempObject;
    ACKError_t err;
    int rawNamespace;
    int rawName;
#ifdef ACK_COMPOUND_DEVICE
    int endpointId;
#endif

    pDirectiveInfo = HeapletAccess(directiveInfoTag);
    ACK_ASSERT(NULL != pDirectiveInfo);

    // Set up access to the DOM Binary Form document's root object.
    DocumentAddressRoot(documentTag, false, &tempObject);

    // Extracts directive object.
    if (!ACKDoc_GetObject(&tempObject, ack_directive_message_member_directive, &directiveObject))
    {
        return ACK_ERR_PROTOCOL;
    }

    // Extracts:
    //  directive.header.namespace
    //  directive.header.name
    if (!ACKDoc_GetObject(&directiveObject, ack_directive_object_member_header, &tempObject)
        || !ACKDoc_GetInt(&tempObject, ack_directive_header_object_member_namespace, &rawNamespace)
        || !ACKDoc_GetInt(&tempObject, ack_directive_header_object_member_name, &rawName)

        // Extracts:
        //  directive.payload
        || !ACKDoc_GetObject(&directiveObject, ack_directive_object_member_payload, &pDirectiveInfo->PayloadObject))
    {
        return ACK_ERR_PROTOCOL;
    }

#ifdef ACK_COMPOUND_DEVICE
    // Extracts endpoint
    if (!ACKDoc_GetObject(&directiveObject, ack_directive_object_member_endpoint, &tempObject)
        || !ACKDoc_GetInt(&tempObject, ack_endpoint_object_member_endpoint_id, &endpointId))
    {
        // Endpoint id is optional, if not present, use default
        endpointId = ACK_COMPOUND_DEVICE_DEFAULT_ENDPOINT;
    }
    SetEndpointId(pDirectiveInfo->CorrelationId, endpointId);
#endif

    pDirectiveInfo->Namespace = (ack_namespace)rawNamespace;
    pDirectiveInfo->Name = rawName;

    switch (pDirectiveInfo->Namespace)
    {
    case ack_namespace_alexa:
        err = ProcessAlexaDirective(pDirectiveInfo);
        break;
        
#ifdef ACK_BRIGHTNESS_CONTROLLER
    case ack_namespace_alexa_brightness_controller:
        err = ProcessBrightnessControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_COLOR_CONTROLLER
    case ack_namespace_alexa_color_controller:
        err = ProcessColorControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_COLOR_TEMPERATURE_CONTROLLER
    case ack_namespace_alexa_color_temperature_controller:
        err = ProcessColorTemperatureControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_COOKING
    case ack_namespace_alexa_cooking:
        err = ProcessCookingDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_COOKING_FOOD_TEMPERATURE_CONTROLLER
    case ack_namespace_alexa_cooking_food_temperature_controller:
        err = ProcessFoodTemperatureControllerCookingDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_COOKING_PRESET_CONTROLLER
    case ack_namespace_alexa_cooking_preset_controller:
        err = ProcessCookingPresetControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_COOKING_TEMPERATURE_CONTROLLER
    case ack_namespace_alexa_cooking_temperature_controller:
        err = ProcessCookingTemperatureControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_COOKING_TIME_CONTROLLER
    case ack_namespace_alexa_cooking_time_controller:
        err = ProcessCookingTimeControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_DEVICE_USAGE
    case ack_namespace_alexa_device_usage_meter:
        err = ProcessDeviceUsageDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_MODE_CONTROLLER
    case ack_namespace_alexa_mode_controller:
        err = ProcessModeControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_PERCENTAGE_CONTROLLER
    case ack_namespace_alexa_percentage_controller:
        err = ProcessPercentageControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_POWER_CONTROLLER
    case ack_namespace_alexa_power_controller:
        err = ProcessPowerControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_RANGE_CONTROLLER
    case ack_namespace_alexa_range_controller:
        err = ProcessRangeControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_THERMOSTAT_CONTROLLER
    case ack_namespace_alexa_thermostat_controller:
        err = ProcessThermostatControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_TIME_HOLD_CONTROLLER
    case ack_namespace_alexa_time_hold_controller:
        err = ProcessTimeHoldControllerDirective(pDirectiveInfo);
        break;
#endif

#ifdef ACK_TOGGLE_CONTROLLER
    case ack_namespace_alexa_toggle_controller:
        err = ProcessToggleControllerDirective(pDirectiveInfo);
        break;
#endif

    default:
#ifdef ACK_OTHER_CAPABILITY
        ACK_DEBUG_PRINT_I("Directive is in unhandled namespace %u.", pDirectiveInfo->Namespace);
        ACK_REENTRY_SHIELDS_UP;
        ACKUser_OnDirective(pDirectiveInfo);
        ACK_REENTRY_SHIELDS_DOWN;
        err = ACK_NO_ERROR;
#else        
        ACK_DEBUG_PRINT_E("Directive is in unhandled namespace %u.", pDirectiveInfo->Namespace);
        err = ACK_ERR_PROTOCOL;
#endif
        break;
    }

    return err;
}

bool CreateEventDocument(ack_namespace namespace, unsigned name, unsigned flags)
{
    ACKDocContainer_t rootObject;
    ACKDocContainer_t eventObject;
    ACKDocContainer_t headerObject;
#ifdef ACK_COMPOUND_DEVICE
    ACKDocContainer_t endpointObject;
#endif

    memset(&g_eventDocument, 0, sizeof(g_eventDocument));

    if (!DocumentInitializeWriteable(MEM_OUTBOUND_BINARY_DOCUMENT, &rootObject)
        || !ACKDoc_AppendObject(&rootObject, ack_event_message_member_event, &eventObject)
        || !ACKDoc_AppendObject(&eventObject, ack_event_object_member_header, &headerObject))
    {
        return false;
    }

#ifdef ACK_COMPOUND_DEVICE
    if (g_eventEndpointId != ACK_COMPOUND_DEVICE_DEFAULT_ENDPOINT)
    {
        if (!ACKDoc_AppendObject(&eventObject, ack_event_object_member_endpoint, &endpointObject)
            || !ACKDoc_AppendInt(&endpointObject, ack_endpoint_object_member_endpoint_id, g_eventEndpointId))
        {
           return false;
        }
    }
#endif

    // Creates:
    //  event.header.namespace
    //  event.header.name
    //  event.payload
    //  context
    return (ACKDoc_AppendInt(&headerObject, ack_event_header_object_member_namespace, namespace)
        && ACKDoc_AppendInt(&headerObject, ack_event_header_object_member_name, name)
        && ACKDoc_AppendObject(&eventObject, ack_event_object_member_payload, &g_eventDocument.PayloadObject)
        && ((flags & CEDF_OMIT_CONTEXT)
            || ACKDoc_AppendObject(&rootObject, ack_event_message_member_context, &g_eventDocument.ContextObject)));
}

// Adds all common values, but not the "value" element.
bool AddPropertyToEventDocument(
    const ACKStateCommon_t* pCommon,
    ack_namespace namespace,
    const uint32_t* pInstance,
    unsigned name,
    ACKDocContainer_t* pPropertyObject)   // returned to caller, for adding actual value
{
    const ACKDocContainer_t* pPropertyContainerObject;
    ACKDocContainer_t changeObject;
    unsigned arrayMember;
    ACKDocContainer_t array;
    uint64_t timeOfSample;

    ACK_CHECK_CAN_ADD_PROPERTY_TO_EVENT(0 != (pCommon->Flags & ACK_PROPERTY_CHANGE_REPORT_PAYLOAD));

    // Accesses one of:
    //  event.payload.change.properties[]
    //  context.properties[]
    if (pCommon->Flags & ACK_PROPERTY_CHANGE_REPORT_PAYLOAD)
    {
        if (!ACKDoc_AppendObject(
            &g_eventDocument.PayloadObject,
            ack_alexa_change_report_payload_object_member_change,
            &changeObject))
        {
            return false;
        }

        pPropertyContainerObject = &changeObject;
        arrayMember = ack_alexa_change_report_change_object_member_properties;
    }
    else
    {
        pPropertyContainerObject = &g_eventDocument.ContextObject;
        arrayMember = ack_context_object_member_properties;
    }

    timeOfSample = pCommon->TimeOfSample;
    if (timeOfSample < ACK_MINIMUM_PROPERTY_TIMESTAMP)
    {
        timeOfSample = ACK_GetApproximateTime();
        if (0 == timeOfSample)
        {
            timeOfSample = ACK_MINIMUM_PROPERTY_TIMESTAMP;
        }
    }
    
    // Ensure there's a properties array in the object, and within the properties array,
    // append a new object with members:
    //  namespace
    //  name
    //  timeOfSample
    //  uncertaintyMilliseconds
    return ACKDoc_AppendArray(pPropertyContainerObject, arrayMember, &array)
        && ACKDoc_AppendObject(&array, 0, pPropertyObject)
        && ACKDoc_AppendInt(pPropertyObject, ack_property_object_member_namespace, namespace)
        && ((NULL == pInstance)
            || ACKDoc_AppendInt64(pPropertyObject, ack_property_object_member_instance, *pInstance))
        && ACKDoc_AppendInt(pPropertyObject, ack_property_object_member_name, name)
        && ACKDoc_AppendInt64(pPropertyObject, ack_property_object_member_time_of_sample, (int64_t)timeOfSample)
        && ACKDoc_AppendInt64(
            pPropertyObject,
            ack_property_object_member_uncertainty_in_milliseconds,
            pCommon->UncertaintyMilliseconds);
}

#ifdef ACK_OTHER_CAPABILITY

bool ACK_AddProperty(
    const ACKStateCommon_t* pCommon,
    ack_namespace propertyNamespace,
    const uint32_t* pPropertyInstance,
    int propertyName,
    ACKDocContainer_t* pPropertyObject)
{
    ACK_REENTRY_CHECK;

    return AddPropertyToEventDocument(pCommon, propertyNamespace, pPropertyInstance, propertyName, pPropertyObject);
}

#endif // def ACK_OTHER_CAPABILITY
