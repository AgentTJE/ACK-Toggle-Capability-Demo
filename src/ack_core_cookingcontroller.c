/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_cooking.h"
#include "ack_core_common.h"
#include "ack_core_cooking_common.h"
#include "ack_core_cooking_mode.h"
#include "ack_core_directives.h"
#include "ack_core_document_wrappers.h"
#include "ack_core_event_builder.h"
#include "ack_core_food_item.h"
#include "ack_core_outbound_events.h"
#include "ack_core_temperature.h"
#include "ack_core_time_interval.h"
#include "ack_user_config.h"
#include <inttypes.h>
#include "ack_generated_ack.h"
#include "ack_generated_alexa_cooking.h"
#include "ack_generated_device_state.h"
#include "ack_generated_required_interaction.h"

#ifdef ACK_COOKING

// Forward declarations.
static ACKError_t StartErrorEvent_(
    int32_t correlationId,
    ack_alexa_cooking_error_type error,
    const char* pOptionalMessage);

#define _NO_SESSION_EVENTS
#ifndef _NO_SESSION_EVENTS
// In 3.x, these don't seem to be used anywhere, the public APIs didn't appear in ack_cooking.h,
// and there were no unit tests for this functionality.
static bool CreateCookingSessionEvent_(ack_alexa_cooking_event_name name, ACKDocContainer_t* pPayloadObject);
static ACKError_t SendCookingSessionEventSendWorker_(void);
#endif

// Calls user-supplied routine to process the directive.
ACKError_t ProcessCookingDirective(const ACKDirectiveParams_t* pDocument)
{
    ACKCookingDirectiveParams_t params;
    CookingDirectiveMembers members = {0};

    // The only directive in this capability is set-cooking-mode.
    switch (pDocument->Name)
    {
    case ack_alexa_cooking_directive_name_set_cooking_mode:
        break;

    default:
        return ACK_ERR_PROTOCOL;
    }

    // Mode required.
    // Food item optional.
    #define _REQUIRED_FIELDS \
        CDFP_MODE_PRESENT | CDFP_MODE_REQUIRED |\
        CDFP_FOODITEM_PRESENT

    members.Mode = ack_alexa_cooking_set_cooking_mode_payload_object_member_cooking_mode;
    members.FoodItem = ack_alexa_cooking_set_cooking_mode_payload_object_member_food_item;

    if (!CookingExtractDirectiveParams(pDocument, _REQUIRED_FIELDS, &members, &params))
    {
        return ACK_ERR_PROTOCOL;
    }

    ACK_REENTRY_SHIELDS_UP;

    ACKUser_OnCookingSetModeDirective(pDocument->CorrelationId, &params);

    ACK_REENTRY_SHIELDS_DOWN;

    return ACK_NO_ERROR;
}

ACKError_t ACK_CompleteDirectiveWithCookingError(
    int32_t correlationId,
    ack_alexa_cooking_error_type error,
    const char* pOptionalMessage)
{
    ACKError_t retval;

    ACK_DEBUG_PRINT_I(
        "Creating cooking error response event, correlationId %"PRId32", error type %u.",
        correlationId,
        error);

    // More specific routine must be used if one is available.
    if (ack_alexa_cooking_error_type_cook_duration_too_long == error)
    {
        ACK_ASSERT(!
            "To report this kind of cooking error, you must call a more specific "
            "ACK_CompleteDirectiveWithCooking*Error routine. See comments in ack_cooking.h.");
    }

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

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    if (ACK_NO_ERROR != retval)
    {
        EventBuilderEnd();
    }

    return retval;
}

ACKError_t ACK_CompleteDirectiveWithCookingDurationTooLongError(
    int32_t correlationId,
    uint64_t maxCookDurationMilliseconds,
    const char* pOptionalMessage)
{
    ACKError_t retval;

    ACK_DEBUG_PRINT_I(
        "Starting cooking duration-too-long error response event, correlationId %"PRId32".",
        correlationId);

    retval = StartErrorEvent_(
        correlationId,
        ack_alexa_cooking_error_type_cook_duration_too_long,
        pOptionalMessage);
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    if (!ACKDoc_AppendInt64(
        &g_eventDocument.PayloadObject,
        ack_alexa_cooking_error_response_payload_object_member_cook_duration_too_long_max_cook_time,
        (int64_t)maxCookDurationMilliseconds))
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

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    if (ACK_NO_ERROR != retval)
    {
        EventBuilderEnd();
    }

    return retval;
}

static ACKError_t StartErrorEvent_(
    int32_t correlationId,
    ack_alexa_cooking_error_type error,
    const char* pOptionalMessage)    
{
    ACKError_t retval;

    OutboundEventReset();

    if (!EventBuilderStart(
        correlationId,
        ack_namespace_alexa_cooking,
        ack_alexa_cooking_event_name_error_response,
        0, // instance not used for this namespace
        EVENT_FLAG_CAN_HAVE_PROPERTIES))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    // Payload: type and message.
    if (!ACKDoc_AppendInt(
            &g_eventDocument.PayloadObject,
            ack_alexa_cooking_error_response_payload_object_member_type,
            error)

        || (pOptionalMessage
            && !ACKDoc_AppendString(
                &g_eventDocument.PayloadObject,
                ack_alexa_cooking_error_response_payload_object_member_message,
                pOptionalMessage)))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    return retval;
}

ACKError_t ACK_AddCookingModeProperty(
    const ACKStateCommon_t* pCommon,
    ack_cooking_mode mode,
    const char* pCustomModeName)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_cooking,
        NULL, // No instance for this capability
        ack_alexa_cooking_property_name_cooking_mode,
        &propertyObject)

        && CookingModeSave(&propertyObject, ack_property_object_member_value, mode, pCustomModeName);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

ACKError_t ACK_AddCookingStatusProperty(
    const ACKStateCommon_t* pCommon,
    ack_cooking_status status)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;
    
    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_cooking,
        NULL, // No instance for this capability
        ack_alexa_cooking_property_name_cooking_status,
        &propertyObject)

        && ACKDoc_AppendInt(&propertyObject, ack_property_object_member_value, status);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

ACKError_t ACK_AddCookingTimeIntervalProperty(
    const ACKStateCommon_t* pCommon,
    uint64_t startTime, // epoch ms
    uint64_t endTime,
    uint64_t durationMilliseconds)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_cooking,
        NULL, // No instance for this capability
        ack_alexa_cooking_property_name_cooking_time_interval,
        &propertyObject)

        && TimeIntervalSave(
            &propertyObject,
            ack_property_object_member_value,
            startTime,
            endTime,
            durationMilliseconds);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

ACKError_t ACK_AddFoodItemProperty(
    const ACKStateCommon_t* pCommon,
    const char* pFoodName,
    const ACKItemQuantity_t* pFoodQuantity)
{
    ACKDocContainer_t propertyObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_cooking,
        NULL, // No instance for this capability
        ack_alexa_cooking_property_name_food_item,
        &propertyObject)
        
        && FoodItemSave(&propertyObject, ack_property_object_member_value, pFoodName, pFoodQuantity);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}

#ifndef _NO_SESSION_EVENTS

ACKError_t ACK_SendCookingSessionCompletedEvent(uint64_t timeOfCompletion)
{
    ACKError_t retval;
    ACKDocContainer_t payloadObject;

    ACK_DEBUG_PRINT_I("Sending cooking session completed event.");

    ACK_REENTRY_CHECK;

    if (!CreateCookingSessionEvent_(ack_alexa_cooking_event_name_cooking_session_completed, &payloadObject)
        || !ACKDoc_AppendInt64(
            &payloadObject,
            ack_alexa_cooking_cooking_session_completed_payload_object_member_time_of_completion,
            (int64_t)timeOfCompletion))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    retval = SendCookingSessionEventSendWorker_();
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    EventBuilderEnd();
    return retval;
}

ACKError_t ACK_SendCookingSessionInteractionEvent(ack_required_interaction postStepInteraction)
{
    ACKError_t retval;
    ACKDocContainer_t payloadObject;

    ACK_DEBUG_PRINT_I("Sending cooking session interaction event.");

    ACK_REENTRY_CHECK;

    if (!CreateCookingSessionEvent_(ack_alexa_cooking_event_name_cooking_session_interaction, &payloadObject)
        || !ACKDoc_AppendInt(
            &payloadObject,
            ack_alexa_cooking_cooking_session_interaction_payload_object_member_required_interaction,
            postStepInteraction))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    retval = SendCookingSessionEventSendWorker_();
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    EventBuilderEnd();
    return retval;
}

ACKError_t ACK_SendCookingDeviceStateChangeEvent(ack_device_state deviceState)
{
    ACKError_t retval;
    ACKDocContainer_t payloadObject;

    ACK_DEBUG_PRINT_I("Sending cooking device state change event.");

    ACK_REENTRY_CHECK;

    if (!CreateCookingSessionEvent_(
        ack_alexa_cooking_event_name_device_state_change,
        &payloadObject)

        || !ACKDoc_AppendInt(
            &payloadObject,
            ack_alexa_cooking_device_state_change_payload_object_member_device_state,
            deviceState))
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    retval = SendCookingSessionEventSendWorker_();
    if (ACK_NO_ERROR != retval)
    {
        goto cleanup0;
    }

    // Success.
    retval = ACK_NO_ERROR;

cleanup0:
    EventBuilderEnd();
    return retval;
}

static bool CreateCookingSessionEvent_(ack_alexa_cooking_event_name name, ACKDocContainer_t* pPayloadObject)
{
    OutboundEventReset();

    // TODO: add endpoint id here, if relevant

    return EventBuilderStart(
        0, // no correlation id
        ack_namespace_alexa_cooking,
        name,
        0, // instance not used for this namespace
        EVENT_FLAGS_NONE);
}

static ACKError_t SendCookingSessionEventSendWorker_(void)
{
    ACKError_t retval;

    g_pendingOutboundEvent.Flags.Bits.EventPending = true;

    retval = OutboundEventSend();
    if (ACK_NO_ERROR != retval)
    {
        ACK_DEBUG_PRINT_E("Error %u sending cooking session event.", retval);
    }

    return retval;
}

#endif // ndef _NO_SESSION_EVENTS

#endif // def ACK_COOKING
