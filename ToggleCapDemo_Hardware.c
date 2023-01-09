/**
@file HelloWorld_Hardware.h
@brief hello world hardware.
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

    ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_7 is the LED light representing global power o(indicating 1 of the other 2 lights is still on).
    ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_14 is the global power button.

    ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_8 is the left LED light.
    ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_15 is the left LED light power button.

    ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_9 is the right LED light.
    ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_16 is the right LED light power button.
*/

#include "src/ack_user_platform.h"
#include "ToggleCapDemo_Hardware.h"

bool g_power = false;
bool g_leftSidePower = false;
bool g_rightSidePower = false;

// The implementations use ACKPlatform_ReadDigitalPin/ACKPlatform_WriteDigitalPin. In a real device,
// you would just touch your hardware directly. The ACKPlatform_xxx routines are used in this sample
// solely to achieve portability.

/* //global variables.
int32_t g_brightness = MAXIMUM_BRIGHTNESS; // Brightness */


void Hardware_SetPowerState(bool powerOn)
{
    g_power = powerOn;
    ACKPlatform_WriteDigitalPin(ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_7, powerOn);
}

bool Hardware_IsPowerOn(void)
{
    return g_power;
}

bool Hardware_IsPowerToggleButtonDown(void)
{
    // Read the state of the power button
    bool powerButtonPressed = ACKPlatform_ReadDigitalPin(ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_14);

    // Return the power button state
    return powerButtonPressed;
}



void Hardware_SetLeftSidePowerState(bool leftSidePowerOn)
{
    g_leftSidePower = leftSidePowerOn;
    ACKPlatform_WriteDigitalPin(ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_8, leftSidePowerOn);
}

bool Hardware_IsLeftSidePowerOn(void)
{
    return g_leftSidePower;
}

bool Hardware_IsLeftSidePowerToggleButtonDown(void)
{
    return ACKPlatform_ReadDigitalPin(ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_15);
}



void Hardware_SetRightSidePowerState(bool rightSidePowerOn)
{
    g_rightSidePower = rightSidePowerOn;
    ACKPlatform_WriteDigitalPin(ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_9, rightSidePowerOn);
}

bool Hardware_IsRightSidePowerOn(void)
{
    return g_rightSidePower;
}

bool Hardware_IsRightSidePowerToggleButtonDown(void)
{
    return ACKPlatform_ReadDigitalPin(ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_16);
}



void Hardware_SetConnectivityLedState(HardwareConnectivityLedState state)
{
    // TODO
}

bool Hardware_IsLifecycleButtonDown(void)
{
    //todo
    return false;
}

/* 
void Hardware_TurnMasterOn(void)
{
    uint8_t brightness = 255;
    ACKPlatform_SetDigitalPinPWMLevel(ACK_HW_PIN_SAMPLE_APPLICATIONS_LED, brightness);
}

void Hardware_TurnMasterOff(void)
{
    ACKPlatform_SetDigitalPinPWMLevel(ACK_HW_PIN_SAMPLE_APPLICATIONS_LED, 0);
}

// Turns left light on.
// This function is hardware specific. In the current implementation it assumes PWM pin
// with 0 as off and 255 as maximum value.
void Hardware_TurnLeftSideOn(void)
{
    uint8_t brightness = 255;
    ACKPlatform_SetDigitalPinPWMLevel(ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_2, brightness);
}

// Turns left light off.
// This function is hardware specific. In the current implementation it assumes PWM pin
// with 0 as off and 255 as maximum value.
void Hardware_TurnLeftSideOff(void)
{
    ACKPlatform_SetDigitalPinPWMLevel(ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_2, 0);
}

// Turns right light on.
// This function is hardware specific. In the current implementation it assumes PWM pin
// with 0 as off and 255 as maximum value.
void Hardware_TurnRightSideOn(void)
{
    uint8_t brightness = 255;
    ACKPlatform_SetDigitalPinPWMLevel(ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_4, brightness);
}

// Turns right light off.
// This function is hardware specific. In the current implementation it assumes PWM pin
// with 0 as off and 255 as maximum value.
void Hardware_TurnRightSideOff(void)
{
    ACKPlatform_SetDigitalPinPWMLevel(ACK_HW_PIN_SAMPLE_APPLICATIONS_GPIO_4, 0);
} */

