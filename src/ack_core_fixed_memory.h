/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "acp.pb.h"

// Memory used transiently for ACP commands and responses.
typedef union _FixedAcpMemory
{
    acp_cmd Command;
    acp_response Response;
}
FixedAcpMemory;

extern FixedAcpMemory g_fixedAcpMemory;

void FixedMemoryWipeAcp(void);

#if 0

#ifdef ACK_ENABLE_ASSERT
#define ACK_FIXED_MEM_DIAG
#endif

#ifdef ACK_FIXED_MEM_DIAG

void FixedMemoryLockAcp(FixedAcpMemoryUse use);
#define ACK_LOCK_ACP_MEMORY(use) FixedMemoryLockAcp(use)

// This operation is idempotent (safe to call when already unlocked).
void FixedMemoryUnlockAcp(FixedAcpMemoryUse use);
#define ACK_UNLOCK_ACP_MEMORY(use) FixedMemoryUnlockAcp(use)

void FixedMemoryCheckAcp(FixedAcpMemoryUse use);
#define ACK_CHECK_ACP_MEMORY(use) FixedMemoryCheckAcp(use)

typedef enum _FixedAcpMemoryUse
{
    FIXED_ACP_MEMORY_IDLE,
    FIXED_ACP_MEMORY_COMMAND,
    FIXED_ACP_MEMORY_RESPONSE
}
FixedAcpMemoryUse;

#else

void FixedMemoryLockAcp(void);

#define ACK_LOCK_ACP_MEMORY(use) FixedMemoryLockAcp()
#define ACK_UNLOCK_ACP_MEMORY(use)
#define ACK_CHECK_ACP_MEMORY(use)

#endif // def ACK_FIXED_MEM_DIAG

#endif
