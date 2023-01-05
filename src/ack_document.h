/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_DOCUMENT_H__
#define __ACK_DOCUMENT_H__

#include "ack_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Opaque struct.
typedef struct _ACKDocContainer_t
{
    unsigned Value;
    uint8_t Id;
    uint8_t Flags;
}
ACKDocContainer_t;

// For these:
//  If pObjectOrArray designates an object, nameOrIndex is an enumerated value representing
//      the name of the desired object member.
//  If pObjectOrArray designates an array, nameOrIndex is the index of the desired array member.
bool ACKDoc_GetIsNull(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, bool* pIsNull);
bool ACKDoc_GetInt(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, int* pValue);
bool ACKDoc_GetInt64(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, int64_t* pValue);
bool ACKDoc_GetDouble(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, double* pValue);
bool ACKDoc_GetFloat(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, float* pValue);
bool ACKDoc_GetBoolean(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, bool* pValue);
bool ACKDoc_GetString(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, const char** ppString);

bool ACKDoc_GetArray(
    const ACKDocContainer_t* pObjectOrArray,
    unsigned nameOrIndex,
    ACKDocContainer_t* pFoundArray);

bool ACKDoc_GetObject(
    const ACKDocContainer_t* pObjectOrArray,
    unsigned nameOrIndex,
    ACKDocContainer_t* pFoundObject);

bool ACKDoc_GetArrayMemberCount(const ACKDocContainer_t* pArray, unsigned* pCount);

// For these:
//  If pObjectOrArray designates an object, nameIfObject is an enumerated value representing the name of
//      the new object member to append to the object.
//  If pObjectOrArray designates an array, nameIfObject is unused and must be 0.
bool ACKDoc_AppendNull(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject);
bool ACKDoc_AppendInt(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, int value);
bool ACKDoc_AppendInt64(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, int64_t value);
bool ACKDoc_AppendDouble(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, double value);
bool ACKDoc_AppendFloat(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, float value);
bool ACKDoc_AppendBoolean(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, bool value);
bool ACKDoc_AppendString(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, const char* pValue);

bool ACKDoc_AppendArray(
    const ACKDocContainer_t* pObjectOrArray,
    unsigned nameIfObject,
    ACKDocContainer_t* pNewArray);

bool ACKDoc_AppendObject(
    const ACKDocContainer_t* pObjectOrArray,
    unsigned nameIfObject,
    ACKDocContainer_t* pNewObject);

#ifdef __cplusplus
}
#endif

#endif // ndef __ACK_DOCUMENT_H__
