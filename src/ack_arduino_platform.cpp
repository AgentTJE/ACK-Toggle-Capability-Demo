/*
 * Copyright 2018-2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

// Implementations of all platform-specific routines (declared in ack_user_platform.h) for Arduino.

#include "ack.h"
#include <Arduino.h>
#include "ack_core_power_saving.h"
#include "ack_crc32.h"
#include <stdarg.h>

// Arduino pins when the Alexa Connect Kit shield is attached to an Arduino Zero.
// Pins 0/1 are used implicitly by Arduino Serial1.
// We use pins 2 and 3 for host interrupt and reset, respectively.
#define ARDUINO_PIN_HOST_INTERRUPT 2
#define ARDUINO_PIN_RESET 3

// Pins for sample applications.

#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_1
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_1 1
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_2
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_2 2
#endif    
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_3
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_3 3
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_4
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_4 4
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_5
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_5 5
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_6
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_6 6
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_7
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_7 7
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_8
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_8 8
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_9
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_9 9
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_10
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_10 10
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_11
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_11 11
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_12
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_12 12
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_13
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_13 13
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_14
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_14 14
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_15
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_15 15
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_16
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_16 16
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_17
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_17 17
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_18
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_18 18
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_19
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_19 19
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_20
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_20 20
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_21
#define ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_21 21
#endif

// Info we need to manage GPIO pins for the ACK Host MCU sample applications.
// Values in the array MUST be in the same order as the ACKHardwareKind_t enum.
static uint32_t sg_arduinoDigitalPins[] =
{
    ARDUINO_PIN_HOST_INTERRUPT,
    ARDUINO_PIN_RESET

#ifdef ACK_MODULE_POWER_SAVING
    ,ACK_MODULE_POWER_SAVING_LOCK_PIN
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_1
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_1
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_2
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_2
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_3
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_3
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_4
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_4
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_5
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_5
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_6
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_6
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_7
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_7
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_8
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_8
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_9
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_9
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_10
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_10
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_11
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_11
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_12
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_12
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_13
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_13
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_14
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_14
#endif    
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_15
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_15
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_16
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_16
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_17
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_17
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_18
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_18
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_19
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_19
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_20
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_20
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_21
    ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_21
#endif
};

// // Info we need to manage GPIO pins for the ACK Host MCU sample applications.
// // Values in the array MUST be in the same order as the ACKHardwareKind_t enum.
// static uint32_t sg_arduinoAnalogPins[] =
// {
// #ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_14
//     ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_14
// #endif    
// #ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_15
//     ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_15
// #endif
// #ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_16
//     ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_16
// #endif
// #ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_17
//     ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_17
// #endif
// #ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_18
//     ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_18
// #endif
// #ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_19
//     ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_19
// #endif
// #ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_20
//     ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_20
// #endif
// #ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_21
//     ,ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_21
// #endif
// }

void ACKPlatform_Initialize()
{
    Serial.begin(115200);
    Serial1.begin(115200);

    pinMode(ARDUINO_PIN_HOST_INTERRUPT, INPUT_PULLUP);
    pinMode(ARDUINO_PIN_RESET, OUTPUT);

#ifdef ACK_MODULE_POWER_SAVING
    // Make sure power saving lock starts unlocked.
    pinMode(ACK_MODULE_POWER_SAVING_LOCK_PIN, OUTPUT);
    digitalWrite(ACK_MODULE_POWER_SAVING_LOCK_PIN, ACK_MODULE_POWER_SAVING_LOCK_ACTIVE ? LOW : HIGH);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_1
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_1, INPUT);
    digitalRead(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_1);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_2
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_2, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_2, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_3
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_3, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_3, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_4
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_4, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_4, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_5
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_5, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_5, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_6
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_6, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_6, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_7
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_7, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_7, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_8
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_8, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_8, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_9
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_9, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_9, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_10
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_10, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_10, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_11
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_11, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_11, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_12
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_12, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_12, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_13
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_13, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_13, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_14
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_14, INPUT);
    digitalRead(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_14);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_15
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_15, INPUT);
    digitalRead(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_15);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_16
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_16, INPUT);
    digitalRead(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_16);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_17
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_17, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_17, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_18
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_18, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_18, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_19
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_19, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_19, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_20
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_20, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_20, LOW);
#endif
#ifdef ACK_SAMPLE_APPLICATIONS_GPIO_PIN_21
    pinMode(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_21, OUTPUT);
    digitalWrite(ARDUINO_PIN_SAMPLE_APPLICATIONS_GPIO_21, LOW);
#endif
}

uint32_t ACKPlatform_TickCount(void)
{
    return millis();
}

void ACKPlatform_Delay(uint32_t milliseconds)
{
    delay(milliseconds);
}

void ACKPlatform_WriteDigitalPin(ACKHardwarePin_t pin, bool state)
{
    digitalWrite(sg_arduinoDigitalPins[pin], state ? HIGH : LOW);
}

// void ACKPlatform_WriteAnalogPin(ACKHardwarePin_t pin, bool state){
//     analogWrite(sg_arduinoAnalogPins[pin], state ? HIGH : LOW);
// }

bool ACKPlatform_ReadDigitalPin(ACKHardwarePin_t pin)
{
    return LOW != digitalRead(sg_arduinoDigitalPins[pin]);
}

// bool ACKPlatform_ReadAnalogPin(ACKHardwarePin_t pin)
// {
//     return LOW != analogRead(sg_arduinoAnalogPins[pin]);
// }

void ACKPlatform_SetDigitalPinPWMLevel(ACKHardwarePin_t pin, uint8_t val)
{
    // Despite the name, analogWrite isn't about digital-to-analog conversion; it's used for PWM.
    analogWrite(sg_arduinoDigitalPins[pin], val);
}

bool ACKPlatform_Send(const void* buffer, size_t length, uint32_t timeoutMilliseconds)
{
    size_t i = 0;
    uint32_t startTime = millis();

    for (i = 0; i < length; ++i)
    {
        if ((millis() - startTime) >= timeoutMilliseconds)
        {
            return false;
        }

        Serial1.write(((const uint8_t*)buffer)[i]);
    }

    return true;
}

bool ACKPlatform_Receive(void* pBuffer, size_t length, uint32_t timeoutMilliseconds)
{
    size_t bytesRead = 0;
    uint32_t startTime = millis();

    // Note that the timeout check is carefully constructed to work even if millis() wraps.
    while ((bytesRead < length) && ((millis() - startTime) < timeoutMilliseconds))
    {
        if (Serial1.available())
        {
            ((uint8_t*)pBuffer)[bytesRead] = Serial1.read();
            bytesRead++;
        }
    }

    return bytesRead == length;
}

void ACKPlatform_DrainRead()
{
    while (Serial1.available())
    {
        Serial1.read();
    }
}

uint32_t ACKPlatform_CalculateCrc32(const void* pInput, size_t length)
{
    return ACK_CalculateCrc32(pInput, length);
}

void ACKPlatform_DebugPrint(const char* pMessage)
{
    Serial.print(pMessage);
}

#ifdef ACK_HOST_FIRMWARE_UPDATE

bool ACKPlatform_StartHostFirmwareUpdate(uint32_t size, uint32_t targetAddress, uint32_t crc32)
{
    return false;
}

bool ACKPlatform_SaveHostFirmwareUpdateChunk(uint32_t startOffset, const uint8_t* pData, uint32_t size)
{
    return false;
}

bool ACKPlatform_HostFirmwareUpdateSuccessfullyRetrieved(void)
{
    return false;
}

void ACKPlatform_HostFirmwareUpdateFailed(void)
{
}

void ACKPlatform_ApplyHostFirmwareUpdate(void)
{
}

#endif // def ACK_HOST_FIRMWARE_UPDATE
