/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_core_alexa_namespace.h"
#include "ack_core_common.h"
#include "ack_core_directives.h"
#include "ack_core_document_wrappers.h"
#include "ack_core_event_builder.h"
#include "ack_core_outbound_events.h"
#include "ack_core_temperature.h"
#include "ack_user_config.h"
#include <inttypes.h>


static ACKError_t StartErrorEvent_(
    int32_t correlationId,
    ack_alexa_error_type error,
    const char* pOptionalMessage);

static ACKError_t ACK_SendChangeReport_(
    ack_alexa_change_report_cause_type cause,
    ACKPropertiesBits_t changedProperiesBits,
    ACKPropertiesBits_t otherPropertiesBits);

ACKError_t ProcessAlexaDirective(const ACKDirectiveParams_t* pDocument)
{
    // The only directive we care about is report-state.
    switch (pDocument->Name)
    {
    case ack_alexa_directive_name_report_state:
        // No payload to worry about for this directive.
        break;
    default:
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnReportStateDirective(pDocument->CorrelationId);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

ACKError_t ACK_CompleteDirectiveWithSuccess(
    int32_t correlationId,
    ACKPropertiesBits_t responsePropertiesBits,
    ACKPropertiesBits_t changeReportPropertiesBits)
{
    ACKError_t retval;

    OutboundEventReset();

    if (!EventBuilderStart(
        correlationId,
        ack_namespace_alexa,
        ack_alexa_event_name_response,
        0, // instance not used in Alexa namespace
        EVENT_FLAG_CAN_HAVE_PROPERTIES))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    g_pendingOutboundEvent.Flags.Bits.EventPending = true;
    g_pendingOutboundEvent.Flags.Bits.SynthesizeChangeReport = true;

    g_pendingOutboundEvent.ChangeReportPayloadPropertiesBits = changeReportPropertiesBits;
    g_pendingOutboundEvent.PropertiesBits = responsePropertiesBits;

    retval = OutboundEventSendIfNotProcessingInboundEvent();
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    if (ACK_NO_ERROR != retval)
    {
        EventBuilderEnd();
    }

    return retval;
}

ACKError_t ACK_CompleteDirectiveWithSimpleError(
    int32_t correlationId,
    ack_alexa_error_type error,
    const char* pOptionalMessage)
{
    ACKError_t retval;

    // More specific routine must be used if one is available.
#ifdef ACK_ENABLE_ASSERT
    bool mustUseMoreSpecific = (ack_alexa_error_type_value_out_of_range == error)
        || (ack_alexa_error_type_temperature_value_out_of_range == error)
        || (ack_alexa_error_type_not_supported_in_current_mode == error);
    if (mustUseMoreSpecific)
    {
        ACK_ASSERT(!
            "To report this kind of error, you must call a more specific ACK_CompleteDirectiveWith*Error "
            "routine. See comments in ack.h.");
    }
#endif // def ACK_ENABLE_ASSERT

    retval = StartErrorEvent_(correlationId, error, pOptionalMessage);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    g_pendingOutboundEvent.Flags.Bits.EventPending = true;

    retval = OutboundEventSendIfNotProcessingInboundEvent();
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Success
    retval = ACK_NO_ERROR;

cleanup0:
    if (ACK_NO_ERROR != retval)
    {
        EventBuilderEnd();
    }

    return retval;
}

ACKError_t ACK_CompleteDirectiveWithOutOfRangeError(
    int32_t correlationId,
    float minimum,
    float maximum,
    const char* pOptionalMessage)
{
    ACKError_t retval;
    ACKDocContainer_t validRangeObject;

    retval = StartErrorEvent_(correlationId, ack_alexa_error_type_value_out_of_range, pOptionalMessage);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    if (!ACKDoc_AppendObject(
            &g_eventDocument.PayloadObject,
            ack_alexa_error_response_payload_object_member_value_out_of_range_valid_range,
            &validRangeObject)

        || !ACKDoc_AppendFloat(
            &validRangeObject,
            ack_alexa_value_out_of_range_error_object_member_minimum_value,
            minimum)

        || !ACKDoc_AppendFloat(
            &validRangeObject,
            ack_alexa_value_out_of_range_error_object_member_maximum_value,
            maximum))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    g_pendingOutboundEvent.Flags.Bits.EventPending = true;

    retval = OutboundEventSendIfNotProcessingInboundEvent();
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Success
    retval = ACK_NO_ERROR;

cleanup0:
    if (ACK_NO_ERROR != retval)
    {
        EventBuilderEnd();
    }

    return retval;
}

ACKError_t ACK_CompleteDirectiveWithTemperatureOutOfRangeError(
    int32_t correlationId,
    ack_temperature_scale scale,
    float minimum,
    float maximum,
    const char* pOptionalMessage)
{
    ACKError_t retval;
    ACKDocContainer_t validRangeObject;

    retval = StartErrorEvent_(
        correlationId,
        ack_alexa_error_type_temperature_value_out_of_range,
        pOptionalMessage);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    if (!ACKDoc_AppendObject(
            &g_eventDocument.PayloadObject,
            ack_alexa_error_response_payload_object_member_temperature_value_out_of_range_valid_range,
            &validRangeObject)

        || !TemperatureSave(
            &validRangeObject,
            ack_alexa_temperature_value_out_of_range_object_member_minimum_value,
            scale,
            minimum)

        || !TemperatureSave(
            &validRangeObject,
            ack_alexa_temperature_value_out_of_range_object_member_maximum_value,
            scale,
            maximum))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    g_pendingOutboundEvent.Flags.Bits.EventPending = true;

    retval = OutboundEventSendIfNotProcessingInboundEvent();
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Success
    retval = ACK_NO_ERROR;

cleanup0:
    if (ACK_NO_ERROR != retval)
    {
        EventBuilderEnd();
    }

    return retval;
}

ACKError_t ACK_CompleteDirectiveWithNotSupportedInCurrentModeError(
    int32_t correlationId,
    ack_alexa_device_mode mode,
    const char* pOptionalMessage)
{
    ACKError_t retval;

    retval = StartErrorEvent_(
        correlationId,
        ack_alexa_error_type_not_supported_in_current_mode,
        pOptionalMessage);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    if (!ACKDoc_AppendInt(
        &g_eventDocument.PayloadObject,
        ack_alexa_error_response_payload_object_member_not_supported_in_current_mode_current_device_mode,
        mode))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    g_pendingOutboundEvent.Flags.Bits.EventPending = true;

    retval = OutboundEventSendIfNotProcessingInboundEvent();
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Success
    retval = ACK_NO_ERROR;

cleanup0:
    if (ACK_NO_ERROR != retval)
    {
        EventBuilderEnd();
    }

    return retval;
}

static ACKError_t StartErrorEvent_(int32_t correlationId, ack_alexa_error_type error, const char* pOptionalMessage)    
{
    ACKError_t retval;

    OutboundEventReset();

    if (!EventBuilderStart(
        correlationId,
        ack_namespace_alexa,
        ack_alexa_event_name_error_response,
        0, // instance not used in Alexa namespace
        EVENT_FLAG_CAN_HAVE_PROPERTIES | EVENT_FLAG_NO_CONTEXT))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    // Payload: type and message.
    if (!ACKDoc_AppendInt(&g_eventDocument.PayloadObject, ack_alexa_error_response_payload_object_member_type, error)
        || !ACKDoc_AppendString(
                &g_eventDocument.PayloadObject,
                ack_alexa_error_response_payload_object_member_message,
                pOptionalMessage != NULL ? pOptionalMessage : ""))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    return retval;
}

ACKError_t ACK_CompleteStateReportWithSuccess(int32_t correlationId)
{
    ACKError_t retval;

    OutboundEventReset();

    if (!EventBuilderStart(
        correlationId,
        ack_namespace_alexa,
        ack_alexa_event_name_state_report,
        0, // instance not used in Alexa namespace
        EVENT_FLAG_CAN_HAVE_PROPERTIES))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    g_pendingOutboundEvent.Flags.Bits.EventPending = true;
    g_pendingOutboundEvent.Flags.Bits.SynthesizeChangeReport = true;
    g_pendingOutboundEvent.Flags.Bits.AddEndpointHealthProperty = true;

#ifdef ACK_COMPOUND_DEVICE
    const ACKPropertyTableEntry_t* pPropertiesMapEntry = ACKUser_PropertyTable;
    uint32_t mapPropertyBit;
    g_pendingOutboundEvent.PropertiesBits = 0;
    uint16_t endpointId = ACK_GetEndpointId(correlationId);

    while (pPropertiesMapEntry->AddPropertiesRoutine)
    {
        mapPropertyBit = ACK_PROPERTY_BIT(pPropertiesMapEntry->PropertyOrdinal);
        
        if (pPropertiesMapEntry->EndpointId == endpointId)
        {
            g_pendingOutboundEvent.PropertiesBits |= mapPropertyBit;
        }
        ++pPropertiesMapEntry;
    }

#else
    g_pendingOutboundEvent.PropertiesBits = ACK_EVENT_ALL_PROPERTIES;
#endif
    g_pendingOutboundEvent.ChangeReportPayloadPropertiesBits = 0;

    retval = OutboundEventSendIfNotProcessingInboundEvent();
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    if (ACK_NO_ERROR != retval)
    {
        EventBuilderEnd();
    }

    return retval;
}

ACKError_t ACK_SendChangeReport(
    ack_alexa_change_report_cause_type cause,
    ACKPropertiesBits_t changedProperiesBits,
    ACKPropertiesBits_t otherPropertiesBits)
{
    ACK_DEBUG_PRINT_I("Sending Alexa change report event, cause %u.", cause);

    ACK_REENTRY_CHECK;

    OutboundEventReset();

#ifdef ACK_COMPOUND_DEVICE
    ACKError_t retval;
    ACKPropertiesBits_t changedByEndpoint;
    ACKPropertiesBits_t otherByEndpoint;
    const ACKPropertyTableEntry_t* pPropertiesMapEntry;
    uint32_t mapPropertyBit;

    if (!changedProperiesBits && !otherPropertiesBits)
    {
        return ACK_SendChangeReport_(cause, changedProperiesBits, otherPropertiesBits);
    }
    else
    {
        // Each loop will send a report with properties for a single endpointId
        while (changedProperiesBits || otherPropertiesBits)
        {
            changedByEndpoint = 0;
            otherByEndpoint = 0;
            pPropertiesMapEntry = ACKUser_PropertyTable;
            g_eventEndpointId = UINT16_MAX; // Endpoint that shouldn't be present in PropertyTable

            // Separate changed bits by endpointId
            while (pPropertiesMapEntry->AddPropertiesRoutine)
            {
                mapPropertyBit = ACK_PROPERTY_BIT(pPropertiesMapEntry->PropertyOrdinal);

                if ((changedProperiesBits & mapPropertyBit) || (otherPropertiesBits & mapPropertyBit))
                {
                    if (g_eventEndpointId == UINT16_MAX)
                    {
                        g_eventEndpointId = pPropertiesMapEntry->EndpointId;
                    }
                    
                    if (pPropertiesMapEntry->EndpointId == g_eventEndpointId)
                    {
                        if (changedProperiesBits & mapPropertyBit) {
                            changedByEndpoint |= mapPropertyBit;
                        }
                        if (otherPropertiesBits & mapPropertyBit) {
                            otherByEndpoint |= mapPropertyBit;
                        }   
                    }
                }
                ++pPropertiesMapEntry;
            }

            retval = ACK_SendChangeReport_(cause, changedByEndpoint, otherByEndpoint);
            if (retval != ACK_NO_ERROR)
            {
                return retval;
            }

            changedProperiesBits ^= changedByEndpoint;
            otherPropertiesBits ^= otherByEndpoint;
        }

        return retval;
    }

#else
    return ACK_SendChangeReport_(cause, changedProperiesBits, otherPropertiesBits);
#endif
}

ACKError_t ACK_SendChangeReport_(
    ack_alexa_change_report_cause_type cause,
    ACKPropertiesBits_t changedProperiesBits,
    ACKPropertiesBits_t otherPropertiesBits)
{
    ACKError_t retval;


    retval = AlexaNamespaceStartChangeReportEvent(cause);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    g_pendingOutboundEvent.ChangeReportPayloadPropertiesBits = changedProperiesBits;
    g_pendingOutboundEvent.PropertiesBits = otherPropertiesBits;

    retval = OutboundEventSend();
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Error %u sending change report event.", retval);
        goto cleanup0;
    }

    retval = ACK_NO_ERROR;

cleanup0:
    EventBuilderEnd();
    return retval;
}

ACKError_t AlexaNamespaceStartChangeReportEvent(ack_alexa_change_report_cause_type cause)
{
    ACKError_t retval;
    ACKDocContainer_t changeObject;
    ACKDocContainer_t changeCauseObject;

    // TODO: add endpoint_id handling

    if (!EventBuilderStart(
            0, // no correlation ID for change reports
            ack_namespace_alexa,
            ack_alexa_event_name_change_report,
            0, // instance not used in Alexa namespace
            EVENT_FLAG_CAN_HAVE_PROPERTIES | EVENT_FLAG_CAN_HAVE_CHANGE_REPORT_PAYLOAD_PROPERTIES)

        // Creates:
        //  event.payload.change.cause.type
        || !ACKDoc_AppendObject(
                &g_eventDocument.PayloadObject,
                ack_alexa_change_report_payload_object_member_change,
                &changeObject)
        || !ACKDoc_AppendObject(
                &changeObject,
                ack_alexa_change_report_change_object_member_cause,
                &changeCauseObject)
        || !ACKDoc_AppendInt(
                &changeCauseObject,
                ack_alexa_change_report_cause_object_member_type,
                cause))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    g_pendingOutboundEvent.Flags.Bits.EventPending = true;
    g_pendingOutboundEvent.Flags.Bits.IsChangeReport = true;
    g_pendingOutboundEvent.Flags.Bits.AddEndpointHealthProperty = true;

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    if (ACK_NO_ERROR != retval)
    {
        EventBuilderEnd();
    }

    return retval;
}
