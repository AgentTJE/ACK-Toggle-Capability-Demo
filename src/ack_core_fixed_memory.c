/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_fixed_memory.h"
#include "ack_types.h"
#include <stdbool.h>
#include <string.h>

FixedAcpMemory g_fixedAcpMemory;

void FixedMemoryWipeAcp(void)
{
    memset(&g_fixedAcpMemory, 0, sizeof(g_fixedAcpMemory));
}

#if 0

#ifdef ACK_FIXED_MEM_DIAG
static FixedAcpMemoryUse sg_acpMemoryInUse = FIXED_ACP_MEMORY_IDLE;

void FixedMemoryLockAcp(FixedAcpMemoryUse use)
{
    ACK_ASSERT(FIXED_ACP_MEMORY_IDLE != use);

    if (FIXED_ACP_MEMORY_IDLE != sg_acpMemoryInUse)
    {
        ACK_ASSERT(!"Logic error: attempt to lock fixed ACP memory while it's already locked.");
    }

    sg_acpMemoryInUse = use;
    memset(&g_fixedAcpMemory, 0, sizeof(g_fixedAcpMemory));
}

void FixedMemoryUnlockAcp(FixedAcpMemoryUse use)
{
    if ((FIXED_ACP_MEMORY_IDLE != sg_acpMemoryInUse) && (sg_acpMemoryInUse != use))
    {
        ACK_ASSERT(!"Logic error: attempt to unlock ACP memory with the wrong use state.");
    }

    sg_acpMemoryInUse = FIXED_ACP_MEMORY_IDLE;
}

void FixedMemoryCheckAcp(FixedAcpMemoryUse use)
{
    if (sg_acpMemoryInUse != use)
    {
        ACK_ASSERT(!"Logic error: fixed ACP memory not in the expected use state.");
    }
}

#else

void FixedMemoryLockAcp(void)
{
    memset(&g_fixedAcpMemory, 0, sizeof(g_fixedAcpMemory));
}

#endif // def ACK_FIXED_MEM_DIAG

#endif
