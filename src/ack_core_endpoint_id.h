/*
 * Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef ASDK_HMCU_ACK_CORE_ENDPOINT_ID_H
#define ASDK_HMCU_ACK_CORE_ENDPOINT_ID_H

#ifdef ACK_COMPOUND_DEVICE

#ifdef __cplusplus
extern "C"
{
#endif

// Can be override to allow support of multiple concurrent directives.
#ifndef ACK_COMPOUND_DEVICE_ARRAY_SIZE
#define ACK_COMPOUND_DEVICE_ARRAY_SIZE 2
#endif

// Can be overriden to specify a different default endpoint.
#ifndef ACK_COMPOUND_DEVICE_DEFAULT_ENDPOINT
#define ACK_COMPOUND_DEVICE_DEFAULT_ENDPOINT 0
#endif

void SetEndpointId(int32_t correlationId, uint16_t endpointId);

#ifdef __cplusplus
}
#endif

#endif

#endif //ASDK_HMCU_ACK_CORE_ENDPOINT_ID_H
