/*
 * Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_core_power_saving.h"
#include "ack_user_platform.h"

#ifdef ACK_MODULE_POWER_SAVING
void LockPowerSaving_(bool longHold, bool locked)
{
    static bool _locked = false;
    static bool _longHold = false;

    if (longHold)
    {
        if (_longHold != locked)
        {
            _longHold = locked;
            ACK_DEBUG_PRINT_D(locked ? "PowerSavingLongLocked" : "PowerSavingLongUnlocked");
        }
    }
     // Acquire the lock at any time, but release it only if there is no long hold
    else if (_locked != locked && (locked || !_longHold))
    {
#ifdef ACK_MODULE_POWER_SAVING_LOCK_ACTIVE_HIGH
        ACKPlatform_WriteDigitalPin(ACK_HW_PIN_MODULE_POWER_SAVING_LOCK, locked);
#else
        ACKPlatform_WriteDigitalPin(ACK_HW_PIN_MODULE_POWER_SAVING_LOCK, !locked);
#endif
        // Add delay so the ACK module has time to enter/exit sleep mode
        ACKPlatform_Delay(ACK_MODULE_POWER_SAVING_LOCK_DELAY);
        _locked = locked;
        ACK_DEBUG_PRINT_D(locked ? "PowerSavingLocked" : "PowerSavingUnlocked");
    }
}
#endif // def ACK_MODULE_POWER_SAVING
