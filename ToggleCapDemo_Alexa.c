/**
@file HelloWorld_Alexa.h
@brief hello world alexa.
@author Tom England
@date 2/22/2022
*/
/*
 * Copyright 2018-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

/*
    Note: this application is a sample only. It may omit functionality that would be required in a
    real device. The application's source code is structured to maximize illustrative utility, and not
    to suggest a reference design for any category of device or to provide a definitive framework for
    a device's firmware.
*/

/*
    This file contains support for integrating ACK into the smart plug. This includes handling for
    Alexa directives, and sending various events such as directive responses, errors, state reports,
    and change reports.
*/

#include "src/ack.h"
#include "src/ack_power_controller.h"
#include "src/ack_user_device.h"
#include "src/ack_brightness_controller.h"
#include "src/ack_toggle_controller.h"
#include "ToggleCapDemo_Alexa.h"
#include "ToggleCapDemo_Hardware.h"


#define __STDC_FORMAT_MACROS // Ensure inttypes.h is functional even when compiled as C++.
#include <inttypes.h>

// Adds the power state property to an outgoing event.
static bool AddPowerStateProperty(uint32_t propertyOrdinal, unsigned propertyFlags);
static bool AddToggleLeftStateProperty(uint32_t propertyOrdinal, unsigned propertyFlags);
static bool AddToggleRightStateProperty(uint32_t propertyOrdinal, unsigned propertyFlags);
static void ProcessSetLeftSideToggleDirective(int32_t correlationId, bool state);
static void ProcessSetRightSideToggleDirective(int32_t correlationId, bool state);

// Property ordinals. Used to map from a property or a set of properties to a routine in our code
// that adds the property or properties to an outgoing event (directive response, state report, or
// change report).
// The actual values here can be anything in the range [0-31].
// For this sample, there's only one property, for the power state.


// Toggle controller instance ids.
// When toggle mode controllers are registered, each is assigned with a unique instance id and must match the device type configuration.
#define TOGGLE_INSTANCE_LEFT_STATE 12
#define TOGGLE_INSTANCE_RIGHT_STATE 13

// Required table that maps properties to routines in our code that populate an outgoing event such as
// a directive response, state report, or change report.
ACKPropertyTableEntry_t ACKUser_PropertyTable[] = 
{
    { POWER_STATE_PROPERTY, AddPowerStateProperty },
    { TOGGLE_LEFT_STATE_PROPERTY, AddToggleLeftStateProperty },
    { TOGGLE_RIGHT_STATE_PROPERTY, AddToggleRightStateProperty },

    // An entry with a NULL callback function terminates the table.
    { 0, NULL }
};

// All Light related properties.
static const ACKPropertiesBits_t c_demoPropertiesBits =
    ACK_PROPERTY_BIT(POWER_STATE_PROPERTY)
    | ACK_PROPERTY_BIT(TOGGLE_LEFT_STATE_PROPERTY)
    | ACK_PROPERTY_BIT(TOGGLE_RIGHT_STATE_PROPERTY);

// Tracks whether the user requested factory reset by long-pressing the lifecycle button, or entering
// user-guided setup by (short-)pressing and then releasing the lifecycle button.
// Factory reset takes precedence.
#define USER_INITIATED_USER_GUIDED_SETUP 1
#define USER_INITIATED_FACTORY_RESET 2
static uint8_t sg_lifecyclePending = 0;
static bool DoesUserWantLifecycleAction(unsigned userInitiated);



//
void Alexa_Setup(void)
{
    // Initialize Host MCU Implementation Core.
    ACK_Initialize();

#ifdef _ACK_EXPERIMENTAL // in ack_user_config.h
    ACKDoc_CreateChangeReportDocument(ack_change_cause_physical_interaction);
    ACKDoc_GetArray(0, 0, 0);
    ACKDoc_GetArrayMemberCount(0, 0);
    ACKDoc_GetBoolean(0, 0, 0);
    ACKDoc_GetDouble(0, 0, 0);
    ACKDoc_GetInt64(0, 0, 0);
    ACKDoc_GetInt(0, 0, 0);
    ACKDoc_GetIsNull(0, 0, 0);
    ACKDoc_GetObject(0, 0, 0);
    ACKDoc_GetString(0, 0, 0);
    ACKDoc_InitializeOutboundDocument(0);
    ACKDoc_AppendArray(0, 0, 0);
    ACKDoc_AppendBoolean(0, 0, 0);
    ACKDoc_AppendDouble(0, 0, 0);
    ACKDoc_AppendInt64(0, 0, 0);
    ACKDoc_AppendNull(0, 0);
    ACKDoc_AppendObject(0, 0, 0);
    ACKDoc_AppendString(0, 0, 0);
#endif
}

//
void Alexa_Loop(void)
{
    // Process incoming ACK events. If there is such an event available, it will have been dispatched
    // to your corresponding handler callback, before ACK_Process returns. If your handler callback
    // calls one of the ACK_Complete* routines, then the response to the event will have also been sent
    // before ACK_Process returns.
    ACK_Process();
}

// Sends an Alexa change report indicating that the power state property changed due to local control
// of the device (e.g. from the keypad).
void Alexa_SendChangeReportDueToLocalControl(void)
{
    // A change report has both changed properties and "related" properties. Since we have only the one
    // property (power state), and we're here to report that it changed, there are never any "related"
    // properties.
    ACK_SendChangeReport(
        ack_alexa_change_report_cause_type_physical_interaction,
        c_demoPropertiesBits,
        0);
}

void Alexa_InitiateFactoryReset(void)
{
    sg_lifecyclePending = USER_INITIATED_FACTORY_RESET;
}

void Alexa_InitiateUserGuidedSetup(void)
{
    // Factory reset takes precedence.
    ACK_STATIC_ASSERT(USER_INITIATED_USER_GUIDED_SETUP < USER_INITIATED_FACTORY_RESET);
    if (sg_lifecyclePending < USER_INITIATED_USER_GUIDED_SETUP)
    {
        sg_lifecyclePending = USER_INITIATED_USER_GUIDED_SETUP;
    }
}

// Returns a value to HMCU ImplCore indicating whether the device is in use.
bool ACKUser_IsDeviceInUse(void)
{
    return Hardware_IsPowerOn();
}

// Handler callback for incoming Alexa.PowerController directives.
// ACK_Process dispatches to this routine if such an incoming event is available from the
// ACK connectivity module when ACK_Process is called.
// Power Controller directive callback, turns on a small LED.
void ACKUser_OnPowerControllerDirective(int32_t correlationId, bool powerOn)
{
    ACKPropertiesBits_t changedPropertiesBits = 0;
    bool changed;

#ifdef ACK_LOGGING
    ACK_WriteLogFormatted(
        acp_log_level_info,
        LOG_COMPONENT_NAME,
        "Handling power directive: %s",
        powerOn ? "ON" : "OFF");
#endif

    ACK_DEBUG_PRINT_I("Controlling LED with power controller directive; light %s", powerOn ? "ON" : "OFF");

    changed = (0 == Hardware_IsPowerOn()) != (0 == powerOn);
    if(changed)
    {
        // In case of power mode change, the logic stops blinking and the timer is stopped.
        /* changedPropertiesBits |= ResetShutdownTimeMode();
        changedPropertiesBits |= ResetBlinkingMode(); */
        changedPropertiesBits |= ACK_PROPERTY_BIT(POWER_STATE_PROPERTY);
        if(powerOn)
        {
            Hardware_SetPowerState(true);

            if(!Hardware_IsLeftSidePowerOn())
            {
                changedPropertiesBits |= ACK_PROPERTY_BIT(TOGGLE_LEFT_STATE_PROPERTY);
                Hardware_SetLeftSidePowerState(true);
                /* g_leftSidePower = true; */
            }
            if(!Hardware_IsRightSidePowerOn())
            {
                changedPropertiesBits |= ACK_PROPERTY_BIT(TOGGLE_RIGHT_STATE_PROPERTY);
                Hardware_SetRightSidePowerState(true);
                /* g_rightSidePower = true; */
            }


        }
        else
        {
            Hardware_SetPowerState(false);

            if(Hardware_IsLeftSidePowerOn()){
                changedPropertiesBits |= ACK_PROPERTY_BIT(TOGGLE_LEFT_STATE_PROPERTY);
                Hardware_SetLeftSidePowerState(false);
            }
            if(Hardware_IsRightSidePowerOn()){
                changedPropertiesBits |= ACK_PROPERTY_BIT(TOGGLE_RIGHT_STATE_PROPERTY);
                Hardware_SetRightSidePowerState(false);
            }
        }
    }

    // Indicate that processing the directive completed successfully,
    // including a change report if and only if the power state actually changed.
    ACK_CompleteDirectiveWithSuccess(
        correlationId,
        c_demoPropertiesBits,
        changedPropertiesBits);
}

void ACKUser_OnToggleControllerDirective(int32_t correlationId, uint32_t instance, bool state)
{
#ifdef ACK_LOGGING
    ACK_WriteLogFormatted(
        acp_log_level_info,
        LOG_COMPONENT_NAME,
        "Handling Toggle directive: %" PRIu32" isDelta: %u value: %d",
        instance,
        isDelta,
        value);
#endif

    switch (instance)
    {
    case TOGGLE_INSTANCE_LEFT_STATE:
        ProcessSetLeftSideToggleDirective(correlationId, state);
        break;
    case TOGGLE_INSTANCE_RIGHT_STATE:
        ProcessSetRightSideToggleDirective(correlationId, state);
        break;
    /* case LEFT_SIDE_PREHEAT:
        ProcessSetLeftSidePreheatToggleDirective(correlationId, state);
        break;
    case RIGHT_SIDE_PREHEAT:
        ProcessSetRightSidePreheatToggleDirective(correlationId, state);
        break; */
    default:
        ACK_DEBUG_PRINT_E("Toggle controller - not supported instance id: %u", instance);
        break;
    }
}

// Handler callback for state report requests.
void ACKUser_OnReportStateDirective(int32_t correlationId)
{
    // Complete the request with success. This causes all properties to be sent.
    ACK_CompleteStateReportWithSuccess(correlationId);
}

// Returns a value to HMCU Reference Implementation Core indicating whether the user has performed the
// magic keypress to initiate factory reset.
bool ACKUser_DoesUserWantFactoryReset(void)
{
    return DoesUserWantLifecycleAction(USER_INITIATED_FACTORY_RESET);
}

// Returns a value to HMCU Reference Implementation Core indicating whether the user has performed the
// magic keypress to initiate setup.
bool ACKUser_DoesUserWantUserGuidedSetup(void)
{
    return DoesUserWantLifecycleAction(USER_INITIATED_USER_GUIDED_SETUP);
}

// Returns a value to HMCU Reference Implementation Core indicating whether the user has performed the
// magic keypress to submit logs. The smart plug has no such keypress, so just return false.
bool ACKUser_DoesUserWantToSubmitLogs(void)
{
    return false;
}

// Updates user-facing connectivity display.
void ACKUser_OnLifecycleStateChange(void)
{
    HardwareConnectivityLedState ledState;

    ACK_DEBUG_PRINT_I("ACK lifecycle state changed to %u", ACK_LifecycleState);

    switch (ACK_LifecycleState)
    {
    case ACK_LIFECYCLE_CONNECTED_TO_ALEXA:
        ledState = HARDWARE_CONNECTIVITY_LED_SOLID;
        break;

    case ACK_LIFECYCLE_FACTORY_RESET_IN_PROGRESS:
    case ACK_LIFECYCLE_NOT_REGISTERED:
    case ACK_LIFECYCLE_IN_SETUP_MODE:
        ledState = HARDWARE_CONNECTIVITY_LED_BLINKING;
        break;

    case ACK_LIFECYCLE_INITIAL:
    case ACK_LIFECYCLE_UNAVAILABLE:
    case ACK_LIFECYCLE_BOOTED:
    case ACK_LIFECYCLE_NOT_CONNECTED_TO_ALEXA:
    default:
        ledState = HARDWARE_CONNECTIVITY_LED_OFF;
        break;

    }

    Hardware_SetConnectivityLedState(ledState);
}




static void ProcessSetLeftSideToggleDirective(int32_t correlationId, bool state){
    ACKPropertiesBits_t changedPropertiesBits = 0;
    bool changed;

    /* bool Hardware_IsPowerOn() = Hardware_IsPowerOn();
    bool Hardware_IsLeftSidePowerOn() = Hardware_IsLeftSidePowerOn(); */
    /* bool Hardware_IsRightSidePowerOn() = Hardware_IsRightSidePowerOn(); */


#ifdef ACK_LOGGING
    ACK_WriteLogFormatted(
        acp_log_level_info,
        LOG_COMPONENT_NAME,
        "Handling left side power toggle directive: %s",
        powerOn ? "ON" : "OFF");
#endif

    ACK_DEBUG_PRINT_I("Controlling left side power with toggle controller directive; light %s", state ? "ON" : "OFF");

    changed = (0 == Hardware_IsLeftSidePowerOn()) != (0 == state);
    if (changed)
    {
        // In case of left side power mode change, the left side timer and heat level are reset to last assigned variables.
        // Preheat will be turned off.
        /* changedPropertiesBits |= ResetLeftShutdownTimeMode();
        changedPropertiesBits |= ResetLeftHeatLevelRange(); */

        // If the input to state: ON, then the left side power state is set to ON. 
        if (state)
        {
            // Change the global power state to ON if it was previously off.
            if(!Hardware_IsPowerOn())
            {
                changedPropertiesBits |= ACK_PROPERTY_BIT(POWER_STATE_PROPERTY);
                Hardware_SetPowerState(true);
            }

            changedPropertiesBits |= ACK_PROPERTY_BIT(TOGGLE_LEFT_STATE_PROPERTY);
            Hardware_SetLeftSidePowerState(true);
        }
        // If the input to state: OFF, then the left side power state is set to OFF. 
        else
        {
            changedPropertiesBits |= ACK_PROPERTY_BIT(TOGGLE_LEFT_STATE_PROPERTY);
            Hardware_SetLeftSidePowerState(false);

            // Change the global power state to OFF if right side power state is also off.
            if(!Hardware_IsRightSidePowerOn())
            {
                changedPropertiesBits |= ACK_PROPERTY_BIT(POWER_STATE_PROPERTY);
                Hardware_SetPowerState(false);
            }
        }

    }

    // Indicate that processing the directive completed successfully,
    // including a change report if and only if the power state actually changed.
    ACK_CompleteDirectiveWithSuccess(
        correlationId,
        c_demoPropertiesBits,
        changedPropertiesBits);
}

static void ProcessSetRightSideToggleDirective(int32_t correlationId, bool state){

    ACKPropertiesBits_t changedPropertiesBits = 0;
    bool changed;

#ifdef ACK_LOGGING
    ACK_WriteLogFormatted(
        acp_log_level_info,
        LOG_COMPONENT_NAME,
        "Handling right side power toggle directive: %s",
        powerOn ? "ON" : "OFF");
#endif

    ACK_DEBUG_PRINT_I("Controlling right side power with toggle controller directive; light %s", state ? "ON" : "OFF");

    changed = (0 == Hardware_IsRightSidePowerOn()) != (0 == state);
    if (changed)
    {
        // In case of right side power mode change, the right side timer and heat level are reset to last assigned variables.
        // Preheat will be turned off.
        /* changedPropertiesBits |= ResetRightShutdownTimeMode();
        changedPropertiesBits |= ResetRightHeatLevelRange(); */
        // If the input to state: ON, then the right side power state is set to ON. 
        if (state)
        {
            // Change the global power state to ON if it was previously off.
            if(!Hardware_IsPowerOn())
            {
                changedPropertiesBits |= ACK_PROPERTY_BIT(POWER_STATE_PROPERTY);
                Hardware_SetPowerState(true);
            }

            changedPropertiesBits |= ACK_PROPERTY_BIT(TOGGLE_RIGHT_STATE_PROPERTY);
            Hardware_SetRightSidePowerState(true);
        }
        // If the input to state: OFF, then the right side power state is set to OFF. 
        else
        {
            changedPropertiesBits |= ACK_PROPERTY_BIT(TOGGLE_RIGHT_STATE_PROPERTY);
            Hardware_SetRightSidePowerState(false);

            // Change the global power state to OFF if left side power state is also off.
            if(!Hardware_IsLeftSidePowerOn())
            {
                changedPropertiesBits |= ACK_PROPERTY_BIT(POWER_STATE_PROPERTY);
                Hardware_SetPowerState(false);
            }
        }

    }

    // Indicate that processing the directive completed successfully,
    // including a change report if and only if the power state actually changed.
    ACK_CompleteDirectiveWithSuccess(
        correlationId,
        c_demoPropertiesBits,
        changedPropertiesBits);
}



// ACK_Process calls this to add properties when it's constructing an outgoing event such as a directive
// response, state report, or change report. The property ordinal is unused because the Power Controller
// capability is single-instance; there can be a maximum of one of them on any device.
static bool AddPowerStateProperty(uint32_t propertyOrdinal, unsigned propertyFlags)
{
    ACKStateCommon_t propertyState = { 0, 0, propertyFlags };
    ACKError_t error;

    error = ACK_AddPowerControllerProperty(&propertyState, Hardware_IsPowerOn());
    if (ACK_NO_ERROR != error)
    {
        ACK_DEBUG_PRINT_E("Error %u adding power state property.", error);
        return false;
    }

    return true;
}

static bool AddToggleLeftStateProperty(uint32_t propertyOrdinal, unsigned propertyFlags)
{
    // Populate metadata about the property.
    // Leave the time-of-sample-field 0 to cause the current time to be sent.
    // Set the error margin to 10 milliseconds for illustrative purposes.
    ACKStateCommon_t common = {0, 10, propertyFlags};
    ACKError_t error;

    error = ACK_AddToggleControllerProperty(TOGGLE_INSTANCE_LEFT_STATE, &common, Hardware_IsLeftSidePowerOn());

    if (ACK_NO_ERROR != error)
    {
        ACK_DEBUG_PRINT_E("Error %u adding left side power state property to event", error);
        return false;
    }

    return true;
}

static bool AddToggleRightStateProperty(uint32_t propertyOrdinal, unsigned propertyFlags)
{
    // Populate metadata about the property.
    // Leave the time-of-sample-field 0 to cause the current time to be sent.
    // Set the error margin to 10 milliseconds for illustrative purposes.
    ACKStateCommon_t common = {0, 10, propertyFlags};
    ACKError_t error;

    error = ACK_AddToggleControllerProperty(TOGGLE_INSTANCE_RIGHT_STATE, &common, Hardware_IsRightSidePowerOn());

    if (ACK_NO_ERROR != error)
    {
        ACK_DEBUG_PRINT_E("Error %u adding right side power state property to event", error);
        return false;
    }

    return true;
}

static bool DoesUserWantLifecycleAction(unsigned userInitiated)
{
    bool userWants;

    userWants = (sg_lifecyclePending == userInitiated);

    if (userWants)
    {
        sg_lifecyclePending = 0;
    }

    return userWants;
}

