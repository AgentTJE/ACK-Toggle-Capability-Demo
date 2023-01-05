/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_CORE_DOCUMENT_H__
#define __ACK_CORE_DOCUMENT_H__

#include "ack_core_heaplet.h"
#include "ack_document.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Element kinds.
//
// MUST fit into 4 bits, because as an important optimization we pack keys with values [1-15] into
// the high nibble of a type byte for object members. If we need more types, we can use
// ACK_DOC_ELEMENT_ESCAPE to indicate the type, and modify the member header parser and emitter in
// ack_core_document.c to understand that when it sees ACKDOC_ELEMENT_ESCAPE as the type, it should load
// the actual type from a subsequent byte.
//
// Also note that ACKDOC_ELEMENT_NONE is not actually used in documents, but is defined here for
// convenience.
#define ACKDOC_ELEMENT_NONE 0               // Not actually used in documents
#define ACKDOC_ELEMENT_OBJECT 1
#define ACKDOC_ELEMENT_ARRAY 2
#define ACKDOC_ELEMENT_STRING 3             // value: bytes of string, including terminating nul
#define ACKDOC_ELEMENT_INTEGER 4            // value: variable-length encoded
#define ACKDOC_ELEMENT_FLOAT64 5            // value: 8 bytes representing an IEEE754 64-bit float
#define ACKDOC_ELEMENT_TRUE 6
#define ACKDOC_ELEMENT_FALSE 7
#define ACKDOC_ELEMENT_NULL 8
#define ACKDOC_ELEMENT_CONTAINER_END 9
#define ACKDOC_ELEMENT_NEGATIVE_INTEGER 10  // value: same as integer, bits flipped (for negative numbers)
#define ACKDOC_ELEMENT_FLOAT32 11           // value: 4 bytes representing an IEEE754 32-bit float
#define ACKDOC_ELEMENT_RESERVED_3 12
#define ACKDOC_ELEMENT_RESERVED_4 13
#define ACKDOC_ELEMENT_RESERVED_5 14
#define ACKDOC_ELEMENT_ESCAPE 15            // Reserved; not currently used

void DocumentAddressRoot(HeapletTag_t documentTag, bool writeable, ACKDocContainer_t* pRootObject);
bool DocumentInitializeWriteable(HeapletTag_t documentTag, ACKDocContainer_t* pRootObject);
void DocumentMakeReadOnly(HeapletTag_t documentTag);

// Indicates that a container itself has a key field.
#define CF_HAS_KEY 0x01

// Indicates that the document in which a container exists is writeable.
#define CF_WRITEABLE 0x02

#ifdef __cplusplus
}
#endif

#endif // ndef __ACK_CORE_DOCUMENT_H__
