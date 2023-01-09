/**
@file HelloWorld_Hardware.h
@brief hello world hardware header.
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

#ifndef __TOGGLE_DEMO_HARDWARE_H__
#define __TOGGLE_DEMO_HARDWARE_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// State of connectivity LED.
typedef enum _HardwareConnectivityLedState
{
    HARDWARE_CONNECTIVITY_LED_OFF,
    HARDWARE_CONNECTIVITY_LED_BLINKING,
    HARDWARE_CONNECTIVITY_LED_SOLID
}
HardwareConnectivityLedState;

void Hardware_SetPowerState(bool powerOn);
bool Hardware_IsPowerOn(void);
bool Hardware_IsPowerToggleButtonDown(void);

void Hardware_SetLeftSidePowerState(bool leftSidePowerOn);
bool Hardware_IsLeftSidePowerOn(void);
bool Hardware_IsLeftSidePowerToggleButtonDown(void);

void Hardware_SetRightSidePowerState(bool rightSidePowerOn);
bool Hardware_IsRightSidePowerOn(void);
bool Hardware_IsRightSidePowerToggleButtonDown(void);

/* void Hardware_TurnMasterOn(void);
void Hardware_TurnMasterOff(void);
void Hardware_TurnLeftSideOn(void);
void Hardware_TurnLeftSideOff(void);
void Hardware_TurnRightSideOn(void);
void Hardware_TurnRightSideOff(void); */

// Added for ACK.
void Hardware_SetConnectivityLedState(HardwareConnectivityLedState state);
bool Hardware_IsLifecycleButtonDown(void);


#ifdef __cplusplus
}
#endif

#endif // ndef __TOGGLE_DEMO_HARDWARE_H__
