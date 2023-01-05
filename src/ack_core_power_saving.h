/*
 * Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_CORE_POWER_SAVING_H__
#define __ACK_CORE_POWER_SAVING_H__

#include "ack_user_config.h"
#include <stdbool.h>

#ifdef ACK_MODULE_POWER_SAVING

// Acquire and release a power saving (PS) lock.
// A short hold is released at the end of ACK_Process, but a long hold needs to be manually released.
// Acquiring a short hold will enable the PS lock.
// Acquiring a long hold will not enable the PS lock, instead it turns into a long hold.
// Releasing a long hold never releases the PS lock, instead it turns into a short hold.
// Releasing a short hold only releases the PS lock if no long hold is acquired.
void LockPowerSaving_(bool longHold, bool locked);

// Short term lock and unlock
#define LOCK_POWER_SAVING() LockPowerSaving_(false, true)
#define UNLOCK_POWER_SAVING() LockPowerSaving_(false, false)

// Long term lock and unlock
#define LONG_LOCK_POWER_SAVING() LockPowerSaving_(true, true)
#define LONG_UNLOCK_POWER_SAVING() LockPowerSaving_(true, false)

#ifndef ACK_MODULE_POWER_SAVING_LOCK_DELAY
#define ACK_MODULE_POWER_SAVING_LOCK_DELAY 20
#endif

// Used for runtime checks of active-high or active-low status.
#ifdef ACK_MODULE_POWER_SAVING_LOCK_ACTIVE_HIGH
#define ACK_MODULE_POWER_SAVING_LOCK_ACTIVE true
#elif defined(ACK_MODULE_POWER_SAVING_LOCK_ACTIVE_LOW)
#define ACK_MODULE_POWER_SAVING_LOCK_ACTIVE false
#else
#error "Feature ACK_MODULE_POWER_SAVING requires either ACK_MODULE_POWER_SAVING_LOCK_ACTIVE_HIGH or ACK_MODULE_POWER_SAVING_LOCK_ACTIVE_LOW"
#endif // def ACK_MODULE_POWER_SAVING_LOCK_ACTIVE_*

#else

#define LOCK_POWER_SAVING()
#define UNLOCK_POWER_SAVING()
#define LONG_LOCK_POWER_SAVING()
#define LONG_UNLOCK_POWER_SAVING()
#endif // def ACK_MODULE_POWER_SAVING

#endif // def __ACK_CORE_POWER_SAVING_H__