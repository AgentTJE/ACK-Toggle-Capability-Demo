/**
@file ack_user_config.h
@brief Configuration file for Alexa Connect Kit. Contains #defines to control its behavior,
    and to determine which features to include when building it.
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
    
*/

#ifndef __ACK_USER_CONFIG_H__
#define __ACK_USER_CONFIG_H__

// Debug print behavior. Leave ACK_DEBUG_PRINT_LEVEL undefined for no debug printing.
#define ACK_DEBUG_PRINT_LEVEL ACK_DEBUG_PRINT_LEVEL_INFO

// Size of buffer from which memory blocks are allocated during processing.
#define ACK_MEMORY_POOL_SIZE 1024

// Turn on to enable asserts in ack functions.
// Recommend leaving off in production to save space.
#define ACK_ENABLE_ASSERT

// Alexa capabilities and other ACK features to support.
#define ACK_POWER_CONTROLLER
#define ACK_TOGGLE_CONTROLLER

// Enable support for the Host MCU sample applications LED pin in the platform-specific implementation.
#define ACK_SAMPLE_APPLICATIONS_LED_PIN
#define ACK_SAMPLE_APPLICATIONS_GPIO_PIN_1
#define ACK_SAMPLE_APPLICATIONS_GPIO_PIN_2
/* #define ACK_SAMPLE_APPLICATIONS_GPIO_PIN_3 */
#define ACK_SAMPLE_APPLICATIONS_GPIO_PIN_4
#define ACK_SAMPLE_APPLICATIONS_GPIO_PIN_5
#define ACK_SAMPLE_APPLICATIONS_GPIO_PIN_6


// Prevent 'stock' empty implementations of some of the device-specific routine in ack_user_device.c
// so we can supply our own.
#define ACK_NO_ACKUSER_DOESUSERWANT
#define ACK_NO_ACKUSER_ONLIFECYCLESTATECHANGE

#define ACK_EXPERIMENTAL

#endif // def __ACK_USER_CONFIG_H__
