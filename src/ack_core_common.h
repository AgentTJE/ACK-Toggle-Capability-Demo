/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_COMMON_H__
#define __ACK_COMMON_H__

#include "ack.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Re-entrancy tracking/checking.
//
// For most re-entrancy tracking/checking, entering/exiting a guarded section of the SDK is a separate operation
// from checking whether attempted entry is allowed. Entering/exiting operations are done with the
// ACK_REENTRY_SHIELDS_UP and _DOWN macros (see below). Checking operations is done with ACK_REENTRY_CHECK.
//
// There are also additional, special-case sections, guarded separately. Those use the more general ACK_REENTRY_ACQUIRE
// and _RELEASE macros along with a more general variant of the _CHECK macro.
//
// Note that this mechanism is not, and does not intend to be, perfect. Instead, it's a best-effort diagnostic.
// One significant factor is that we don't have a good way of performing atomic operations on the variable we use
// to track which categories of non-reentrant code are currently being guarded (see below). We can't assume an RTOS
// with a mutex API, or a toolchain with atomic compiler intrinsics that could help. And even with some toolchains
// that do have support for atomic compiler intrinsics, the CRT doesn't actually implement them. For example,
// __atomic_xxx on gcc (https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html) compile when building for
// embedded, but result in undefined symbol link errors. We could invent a spinlock mechanism to try to deal with this
// but that's out of scope in this best-effort approach in an SDK explicitly designed for single-threaded/non-RTOS
// operation (or more exactly, designed so that if used otherwise, the application can provide any required
// synchronization).
//
// The different categories of guarded code are identified by these macros.
#define ACK_REENTRY_GENERAL_CASE    0x01   // General case
#define ACK_SIMULTANEOUS_ACKPROCESS 0x02   // Special case for ACK_Process()

#ifdef ACK_ENABLE_ASSERT

extern volatile uint8_t g_reentryGuard;

void ReentryFail(const char* pFunctionName);

// Macros for re-entrancy tracking/checking for the general case.
#define ACK_REENTRY_SHIELDS_UP ACK_REENTRY_ACQUIRE_(ACK_REENTRY_GENERAL_CASE)
#define ACK_REENTRY_SHIELDS_DOWN ACK_REENTRY_RELEASE_(ACK_REENTRY_GENERAL_CASE)
#define ACK_REENTRY_CHECK ACK_REENTRY_CHECK_(ACK_REENTRY_GENERAL_CASE)

// Worker macros. Can also be used to guard code other than the general case.
#define ACK_REENTRY_ACQUIRE_(category) g_reentryGuard |= (category)
#define ACK_REENTRY_RELEASE_(category) g_reentryGuard &= ~(category)
#define ACK_REENTRY_CHECK_(category) if (0 != (g_reentryGuard & (category))) ReentryFail(__FUNCTION__)

#else

#define ACK_REENTRY_SHIELDS_UP
#define ACK_REENTRY_SHIELDS_DOWN
#define ACK_REENTRY_CHECK

#define ACK_REENTRY_ACQUIRE_(category)
#define ACK_REENTRY_RELEASE_(category)
#define ACK_REENTRY_CHECK_(category)

#endif

#ifdef __cplusplus
}
#endif

#endif // def __ACK_COMMON_H__
