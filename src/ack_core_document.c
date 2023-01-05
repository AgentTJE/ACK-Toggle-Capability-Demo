/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_document.h"
#include "ack_core_heaplet.h"
#include "ack_debug_print.h"
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
    Document Object Model (DOM) API implementation.

    The DOM API provides access to documents with a structure logically similar to JSON. It operates on
    DOM Binary Form documents, which consist of containers (objects and arrays) and members (integers,
    floats, booleans, strings, null, as well as nested containers). The API and its implementation are both
    designed to carefully balance implementation code size, runtime CPU usage, and the size of DOM Binary
    Form documents.

    API Shape
    ---------

    The API is logically split into two sections: Get and Append. The Get API (ACKDoc_Get*) provides access
    to any member of any object or array, by key (integer) within objects and by 0-based index within arrays.
    To use the Get API, a consumer must know the type of value to retrieve; the DOM implementation provides
    no conversions. This means for example that if a value is semantically integral, the consumer must
    use ACKDoc_GetInt or ACKDoc_GetInt64; all other Get APIs would fail to retrieve such a value. This helps
    avoid hidden code size penalties in the implementation. For example converting between int64 and double
    can drag in 4K of code from the C Runtime library on ARM Cortex M0 -- which is more than double the size
    of the entire implementation without such conversions.

    The Append API (ACKDoc_Append*) is used to append a member to an object or an array. Append is
    the only write operation; there is no way to, for example, insert at a given position in an array. That
    functionality is simply not needed, and to keep the code as small as possible it is therefore not
    provided.

    Document Kinds
    --------------

    Two kinds of documents are supported: read-only and writeable. Read-only documents are used for messages
    that arrive from Alexa to the device, i.e. directives. Writeable documents are used for messages that
    are sent from the device to Alexa, i.e. events. The DOM Get API is for both kinds of documents. The DOM
    Append API operates only on documents explicitly created as writeable. This is because the implementation
    maintains an associated index for these documents (described below).

    DOM Binary Form Documents
    -------------------------

    A DOM Binary Form document consists of a root-level object containing members, including other
    containers nested to an arbitrary depth. Each member is as follows:

    Type: ACKDOC_ELEMENT_*, low 4 bits of 1 byte (see #defines in ack_core_document.h).
    Key: only present for object members; integer-encoded (see below)
    Value: see below

    Values:
        #define ACKDOC_ELEMENT_INTEGER           1 // integer-encoded value (see below *)
        #define ACKDOC_ELEMENT_FLOAT64           2 // value is 8 bytes, little-endian
        #define ACKDOC_ELEMENT_STRING            3 // value is bytes of UTF-8 string, including nul terminator
        #define ACKDOC_ELEMENT_OBJECT            4 // 'value' is packed elements (see above)
        #define ACKDOC_ELEMENT_ARRAY             5 // 'value' is packed elements (see above)
        #define ACKDOC_ELEMENT_TRUE              6 // no explicit value bytes needed
        #define ACKDOC_ELEMENT_FALSE             7 // no explicit value bytes needed
        #define ACKDOC_ELEMENT_NULL              8 // no explicit value bytes needed
        #define ACKDOC_ELEMENT_CONTAINER_END     9 // no type byte and no value bytes
        #define ACKDOC_ELEMENT_NEGATIVE_INTEGER 10 // bit-flipped then integer-encoded value
        #define ACKDOC_ELEMENT_FLOAT32          11 // value is 4 bytes, little-endian

    * Integer-encoded: groups of 7 bits, with high bit in a byte indicating whether it's the final
                       byte in the encoded integer (set: intermediate byte, clear: final byte).
                       Little-endian (low 7 bits in first byte).

    ACKDOC_ELEMENT_CONTAINER_END (9) indicates the end of containers. This consists solely of the type byte;
    there is never a key or value.

    As an additional important optimization, for object member elements a key in the range 1-15 is stored in
    the upper 4 bits of the type byte. A value of 0 means that the key is stored explicitly as shown above.
    
    Consider this example JSON-like string (it's valid JSON, except that in DOM Binary Form object member
    keys are integers instead of strings):

        { 1: { 1: [2.5, true, null], 2: "xyz", 3: 150, 20: false } }
        ^    ^    ^
        |    |    |
        |    |    +--- Array (nested in Object B)
        |    |
        |    +--- Object B (nested in root)
        |
        +--- Root object

    The DOM Binary form for this document is as follows:

        01                                      Root object (no key for root; high nibble always 0)
           11                                   Root Object member, key = 1, value = Object B
              12                                Object B member, key = 1, value = Array
                 05 00 00 00 00 00 00 04 40     Array member, 64-bit double, value = 2.5
                 06                             Array member, value = true
                 08                             Array member, value = null
              09                                Array end
              23 78 79 7A 00                    Object B member, key = 2, value = "xyz"
              34 96 01                          Object B member, key = 3, value = 150
              07 14                             Object B member, key = 20 (stored explicitly), value = false
           09                                   Object B end
        09                                      Root object end

    Object/Array Identity
    ---------------------

    All DOM APIs operate on a particular object or array, either to retrieve a member's value or to append
    a new member. For read-only documents, the "identity" of a container (in ACKDocContainer_t) is simply
    its offset within the DOM Binary Form document. In a read-only document, this value never changes, and
    so handing it out to consumers is safe.

    For writeable documents, appending members changes the offset of containers higher in memory than
    the insertion point, which makes handing out offsets as container identity unsafe. Instead, we maintain
    a separate index to map between container offsets and ordinals, and use a container's ordinal in the
    index as its identity. Whenever an insertion occurs, the index is updated such that the stored offsets
    for affected containers are adjusted by the count of bytes inserted into the document. The index is
    thrown away before encoding a document as the payload into an ACP send-event command, helping to keep
    RAM requirements low.
    
    Manipulating writeable documents is somewhat less efficient than the more direct offset-based access
    for read-only documents, but the index mechanism allows read-only and writeable DOM Binary Form documents
    to share a svelte representation and (mostly) implementation.

    Document Storage
    ----------------

    There is a 1:1 correlation between a DOM Binary Form document in this implementation and a heaplet
    block determined by a consumer. For read-only documents, the block is sized to exactly fit the document.
    For writeable documents, a header and the index is stored in the same block, ahead of the document in
    memory. The header stores housekeeping info associated with the document such as the count of entries
    in the container offset index. Placing the index at the start of the block avoids worries about
    data alignment issues for the index entries (e.g. if the binary data for the document itself is of odd
    length).
*/

// Data structure for writeable documents. Such documents are stored with this header followed by the
// container index and then the document data itself.
typedef struct _WriteableDocumentHeader_t
{
    unsigned IndexEntryCount;
}
WriteableDocumentHeader_t;

typedef uint16_t ContainerIndexEntry_t;

typedef struct _DocumentContext_t
{
    // Pointer to the base of the document's heaplet block, and size of the heaplet block.
    // Updated upon entry to a public API, from the tag in an ACKDocContainer_t.
    void* BlockBase;
    unsigned BlockSize;

    // An offset within the heaplet block.
    unsigned Offset;
}
DocumentContext_t;

typedef enum _Access_t
{
    DBF_ACCESS_FIND,
    DBF_ACCESS_COUNT,
    DBF_ACCESS_APPEND
}
Access_t;

typedef enum _Found_t
{
    DBF_FOUND_IN_OBJECT,
    DBF_FOUND_IN_ARRAY,
    DBF_NOT_IN_OBJECT,
    DBF_NOT_IN_ARRAY,
}
Found_t;

typedef struct _EncodedInteger_t
{
    // 64-bit value expressed as 7-bit groups requires at most 10 bytes.
    uint8_t Bytes[10];
    unsigned ByteCount;
}
EncodedInteger_t;

typedef struct _FloatingPoint_t
{
    double Double;
    float Float;
}
FloatingPoint_t;

static bool GetFloat_(
    const ACKDocContainer_t* pContainer,
    unsigned nameOrIndex,
    FloatingPoint_t* pValue,
    bool float64);

static bool GetContainer_(
    const ACKDocContainer_t* pParentContainer,
    unsigned keyOrIndex,
    unsigned expectedType,
    ACKDocContainer_t* pFound);

static bool CreateContainer_(
    const ACKDocContainer_t* pParentContainer,
    unsigned key,
    uint8_t type,
    ACKDocContainer_t* pNewContainer);

static bool GetMember_(
    const ACKDocContainer_t* pParentContainer,
    unsigned key,
    unsigned* pFoundType,
    Found_t* pFound,
    DocumentContext_t* pContext);

static bool AppendMember_(
    const ACKDocContainer_t* pParentContainer,
    unsigned key,
    uint8_t type,
    const void* pData,
    unsigned dataSize,
    ACKDocContainer_t* pContainerOut);

static bool AccessMember_(
    const ACKDocContainer_t* pParentContainer,
    Access_t access,
    unsigned memberKey,
    unsigned* pFoundMemberType,
    DocumentContext_t* pContext,
    Found_t* pFound);

static bool AccessContainer_(
    const ACKDocContainer_t* pContainer,
    DocumentContext_t* pContext,
    bool* pIsObject);

static bool LoadMemberHeader_(DocumentContext_t* pContext, unsigned* pType, unsigned* pKey);
static bool SkipContainerBody_(DocumentContext_t* pContext, bool membersHaveKeys);
static bool SkipNonContainerMemberValue_(DocumentContext_t* pContext, unsigned memberType);

static bool LoadEncodedInteger_(DocumentContext_t* pContext, int64_t* pValue, bool flipBits);
static bool LoadEncodedSmallUnsignedInteger_(DocumentContext_t* pContext, unsigned* pValue);
static void EncodeInteger_(int64_t value, EncodedInteger_t* pEncoded, bool flipBits);
static bool LoadFloat64_(DocumentContext_t* pContext, double* pValue);
static bool LoadFloat32_(DocumentContext_t* pContext, float* pValue);
static bool LoadString_(DocumentContext_t* pContext, const char** ppValue);
static int LoadByte_(DocumentContext_t* pContext);
static bool LoadLittleEndian_(DocumentContext_t* pContext, uint8_t* pData, unsigned count);
static void SwapOrderIfBigEndian_(uint8_t* pData, unsigned dataSize);
static bool IsBigEndian_(void);

#define ACK_IS_CONTAINER(t) ((ACKDOC_ELEMENT_OBJECT == (t)) || (ACKDOC_ELEMENT_ARRAY== (t)))

void DocumentAddressRoot(HeapletTag_t documentTag, bool writeable, ACKDocContainer_t* pRootObject)
{
    pRootObject->Id = documentTag;
    pRootObject->Value = 0;

    // Root object member header does not have a key, so don't set CF_HAS_KEY.
    pRootObject->Flags = writeable ? CF_WRITEABLE : 0;
}

bool DocumentInitializeWriteable(HeapletTag_t documentTag, ACKDocContainer_t* pRootObject)
{
    WriteableDocumentHeader_t* pHeader;
    ContainerIndexEntry_t* pIndexEntry;
    uint8_t* pRootBytes;

    // Allocate space for a header, one entry in the document's container index for the root object,
    // and the empty root object itself.
    pHeader = HeapletSetSize(
        documentTag,
        sizeof(WriteableDocumentHeader_t) + sizeof(ContainerIndexEntry_t) + 2);
    if (NULL == pHeader)
    {
        ACK_DEBUG_PRINT_E("Initialize writeable: allocating block %u failed", documentTag);
        return false;
    }

    pIndexEntry = (ContainerIndexEntry_t*)(pHeader + 1);
    pRootBytes = (uint8_t*)(pIndexEntry + 1);

    pHeader->IndexEntryCount = 1;
    *pIndexEntry = sizeof(WriteableDocumentHeader_t) + sizeof(ContainerIndexEntry_t);
    pRootBytes[0] = ACKDOC_ELEMENT_OBJECT;
    pRootBytes[1] = ACKDOC_ELEMENT_CONTAINER_END;

    pRootObject->Value = 0; // Root is container ordinal 0
    pRootObject->Id = documentTag;
    pRootObject->Flags = CF_WRITEABLE;
    return true;
}

void DocumentMakeReadOnly(HeapletTag_t documentTag)
{
    void* pBlockData;
    size_t blockSize;
    size_t nonBodySize;
    size_t bodySize;
    WriteableDocumentHeader_t* pHeader;

    if (HeapletGetBlock(documentTag, &pBlockData, &blockSize))
    {
        // Trim out the index.
        pHeader = pBlockData;
        nonBodySize = sizeof(WriteableDocumentHeader_t)
            + (pHeader->IndexEntryCount * sizeof(ContainerIndexEntry_t));
        bodySize = blockSize - nonBodySize;

        memmove(pBlockData, (uint8_t*)pBlockData + nonBodySize, bodySize);

        HeapletSetSize(documentTag, bodySize);
    }
}

bool ACKDoc_GetIsNull(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, bool* pIsNull)
{
    DocumentContext_t context;
    unsigned type;

    if (!GetMember_(pObjectOrArray, nameOrIndex, &type, NULL, &context))
    {
        return false;
    }

    *pIsNull = (ACKDOC_ELEMENT_NULL == type);
    return true;
}

bool ACKDoc_GetInt(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, int* pValue)
{
    int64_t value;

    if (!ACKDoc_GetInt64(pObjectOrArray, nameOrIndex, &value))
    {
        return false;
    }

    // Clamp to int range.
    if (value < INT_MIN)
    {
        *pValue = INT_MIN;
    }
    else if (value > INT_MAX)
    {
        *pValue = INT_MAX;
    }
    else
    {
        *pValue = (int)value;
    }

    return true;
}

bool ACKDoc_GetInt64(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, int64_t* pValue)
{
    DocumentContext_t context;
    unsigned type;

    if (!GetMember_(pObjectOrArray, nameOrIndex, &type, NULL, &context)
        || ((ACKDOC_ELEMENT_INTEGER != type) && (ACKDOC_ELEMENT_NEGATIVE_INTEGER != type)))

    {
        return false;
    }

    return LoadEncodedInteger_(&context, pValue, ACKDOC_ELEMENT_NEGATIVE_INTEGER == type);
}

bool ACKDoc_GetDouble(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, double* pValue)
{
    FloatingPoint_t floatingPoint;
    bool result;

    result = GetFloat_(pObjectOrArray, nameOrIndex, &floatingPoint, true);
    if (result)
    {
        *pValue = floatingPoint.Double;
    }

    return result;
}

bool ACKDoc_GetFloat(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, float* pValue)
{
    FloatingPoint_t floatingPoint;
    bool result;

    result = GetFloat_(pObjectOrArray, nameOrIndex, &floatingPoint, false);
    if (result)
    {
        *pValue = floatingPoint.Float;
    }

    return result;
}

static bool GetFloat_(
    const ACKDocContainer_t* pContainer,
    unsigned nameOrIndex,
    FloatingPoint_t* pValue,
    bool float64)
{
    DocumentContext_t context;
    unsigned type;
    bool result;

    result = GetMember_(pContainer, nameOrIndex, &type, NULL, &context);
    if (result)
    {
        switch (type)
        {
        case ACKDOC_ELEMENT_FLOAT32:
            result = LoadFloat32_(&context, &pValue->Float);
            pValue->Double = pValue->Float;
            break;

        case ACKDOC_ELEMENT_FLOAT64:
            result = LoadFloat64_(&context, &pValue->Double);
            pValue->Float = (float)pValue->Double;
            break;

        default:
            result = false;
            break;
        }
    }

    return result;
}

bool ACKDoc_GetBoolean(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, bool* pValue)
{
    DocumentContext_t context;
    unsigned type;
    bool result;

    result = GetMember_(pObjectOrArray, nameOrIndex, &type, NULL, &context);
    if (result)
    {
        if (ACKDOC_ELEMENT_TRUE == type)
        {
            *pValue = true;
        }
        else if (ACKDOC_ELEMENT_FALSE == type)
        {
            *pValue = false;
        }
        else
        {
            result = false;
        }
    }

    return result;
}

bool ACKDoc_GetString(const ACKDocContainer_t* pObjectOrArray, unsigned nameOrIndex, const char** ppString)
{
    DocumentContext_t context;
    unsigned type;

    if (!GetMember_(pObjectOrArray, nameOrIndex, &type, NULL, &context)
        || (ACKDOC_ELEMENT_STRING != type))
    {
        return false;
    }

    *ppString = (const char*)((uintptr_t)context.BlockBase + context.Offset);
    return true;
}

bool ACKDoc_GetArray(
    const ACKDocContainer_t* pObjectOrArray,
    unsigned nameOrIndex,
    ACKDocContainer_t* pFoundArray)
{
    return GetContainer_(pObjectOrArray, nameOrIndex, ACKDOC_ELEMENT_ARRAY, pFoundArray);
}

bool ACKDoc_GetObject(
    const ACKDocContainer_t* pObjectOrArray,
    unsigned nameOrIndex,
    ACKDocContainer_t* pFoundObject)
{
    return GetContainer_(pObjectOrArray, nameOrIndex, ACKDOC_ELEMENT_OBJECT, pFoundObject);
}

static bool GetContainer_(
    const ACKDocContainer_t* pParentContainer,
    unsigned keyOrIndex,
    unsigned expectedType,
    ACKDocContainer_t* pFound)
{
    DocumentContext_t context;
    unsigned type;
    Found_t found;

    if (!GetMember_(pParentContainer, keyOrIndex, &type, &found, &context)
        || (type != expectedType))
    {
        return false;
    }

    pFound->Value = context.Offset;
    pFound->Id = pParentContainer->Id;
    pFound->Flags = (DBF_FOUND_IN_OBJECT == found) ? CF_HAS_KEY : 0;
    return true;
}

bool ACKDoc_GetArrayMemberCount(const ACKDocContainer_t* pArray, unsigned* pCount)
{
    DocumentContext_t context;
    unsigned count;
    bool result;
    Found_t found;

    result = AccessMember_(pArray, DBF_ACCESS_COUNT, 0, &count, &context, &found);
    if (result)
    {
        *pCount = count;
    }

    return result;
}

bool ACKDoc_AppendNull(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject)
{
    return AppendMember_(pObjectOrArray, nameIfObject, ACKDOC_ELEMENT_NULL, NULL, 0, NULL);
}

bool ACKDoc_AppendInt(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, int value)
{
    return ACKDoc_AppendInt64(pObjectOrArray, nameIfObject, value);
}

bool ACKDoc_AppendInt64(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, int64_t value)
{
    EncodedInteger_t encodedInteger;

    EncodeInteger_(value, &encodedInteger, value < 0);

    return AppendMember_(
        pObjectOrArray,
        nameIfObject,
        (value < 0) ? ACKDOC_ELEMENT_NEGATIVE_INTEGER : ACKDOC_ELEMENT_INTEGER,
        encodedInteger.Bytes,
        encodedInteger.ByteCount,
        NULL);
}

bool ACKDoc_AppendDouble(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, double value)
{
    ACK_STATIC_ASSERT(sizeof(value) == 8);
    SwapOrderIfBigEndian_((uint8_t*)&value, sizeof(value));

    return AppendMember_(pObjectOrArray, nameIfObject, ACKDOC_ELEMENT_FLOAT64, &value, sizeof(value), NULL);
}

bool ACKDoc_AppendFloat(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, float value)
{
    ACK_STATIC_ASSERT(sizeof(value) == 4);
    SwapOrderIfBigEndian_((uint8_t*)&value, sizeof(value));

    return AppendMember_(pObjectOrArray, nameIfObject, ACKDOC_ELEMENT_FLOAT32, &value, sizeof(value), NULL);
}

bool ACKDoc_AppendBoolean(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, bool value)
{
    return AppendMember_(
        pObjectOrArray,
        nameIfObject,
        value ? ACKDOC_ELEMENT_TRUE : ACKDOC_ELEMENT_FALSE,
        NULL,
        0,
        NULL);
}

bool ACKDoc_AppendString(const ACKDocContainer_t* pObjectOrArray, unsigned nameIfObject, const char* pValue)
{
    return AppendMember_(
        pObjectOrArray,
        nameIfObject,
        ACKDOC_ELEMENT_STRING,
        pValue,
        strlen(pValue) + 1,
        NULL);
}

bool ACKDoc_AppendArray(
    const ACKDocContainer_t* pObjectOrArray,
    unsigned nameIfObject,
    ACKDocContainer_t* pNewArray)
{
    return CreateContainer_(pObjectOrArray, nameIfObject, ACKDOC_ELEMENT_ARRAY, pNewArray);
}

bool ACKDoc_AppendObject(
    const ACKDocContainer_t* pObjectOrArray,
    unsigned nameIfObject,
    ACKDocContainer_t* pNewObject)
{
    return CreateContainer_(pObjectOrArray, nameIfObject, ACKDOC_ELEMENT_OBJECT, pNewObject);
}

static bool CreateContainer_(
    const ACKDocContainer_t* pParentContainer,
    unsigned key,
    uint8_t type,
    ACKDocContainer_t* pNewContainer)
{
    bool result;
    uint8_t endOfContainer;

    // Treat the end-of-container member as the "value" of the container. This avoids a special-case
    // inside AppendMember_.
    endOfContainer = ACKDOC_ELEMENT_CONTAINER_END;
    result = AppendMember_(pParentContainer, key, type, &endOfContainer, 1, pNewContainer);

    if (!result)
    {
        ACK_DEBUG_PRINT_E("Create container: append failed");
        return false;
    }

    return true;
}

static bool GetMember_(
    const ACKDocContainer_t* pParentContainer,
    unsigned key,
    unsigned* pFoundType,
    Found_t* pFound,
    DocumentContext_t* pContext)
{
    Found_t found;

    if (!AccessMember_(pParentContainer, DBF_ACCESS_FIND, key, pFoundType, pContext, &found))
    {
        return false;
    }

    if (pFound)
    {
        *pFound = found;
    }

    return (DBF_FOUND_IN_OBJECT == found) || (DBF_FOUND_IN_ARRAY == found);
}

static bool AppendMember_(
    const ACKDocContainer_t* pParentContainer,
    unsigned key,
    uint8_t type,
    const void* pData,
    unsigned dataSize,
    ACKDocContainer_t* pContainerOut)
{
    Found_t found;
    unsigned foundType;
    DocumentContext_t context;
    unsigned insertionSize;
    unsigned newEntrySize;
    EncodedInteger_t encodedKey;
    uint8_t* pNewMember;
    WriteableDocumentHeader_t* pHeader;
    ContainerIndexEntry_t* pIndex;
    unsigned i;
    unsigned entryCount;

    // Make sure the document is writeable before doing anything, and then find the member if it
    // already exists.
    if ((0 == (pParentContainer->Flags & CF_WRITEABLE))
        || !AccessMember_(pParentContainer, DBF_ACCESS_APPEND, key, &foundType, &context, &found))
    {
        ACK_DEBUG_PRINT_E(
            "Append member: document %u is not writeable, or access member failed",
            pParentContainer->Id);
        return false;
    }

    pHeader = context.BlockBase;
    pIndex = (ContainerIndexEntry_t*)(pHeader + 1);

    // If the member already exists, bail out early. Note that this can happen only for objects,
    // because appending for arrays always adds a new item, whereas an object's members are named.
    // We expect context to point at the member header when we get here.
    if (DBF_FOUND_IN_OBJECT == found)
    {
        // Appending containers that already exist is allowed.
        // Appending values that already exist is not allowed.
        if (ACK_IS_CONTAINER(type))
        {
            // Find ordinal from offset.
            foundType = 0;
            for (i = 0; i < pHeader->IndexEntryCount; ++i)
            {
                if (pIndex[i] == context.Offset)
                {
                    foundType = i;
                    break;
                }
            }

            if (!foundType)
            {
                ACK_DEBUG_PRINT_E("Append member: no container at offset %u", context.Offset);
                return false;
            }

            pContainerOut->Value = foundType;
            pContainerOut->Id = pParentContainer->Id;
            pContainerOut->Flags = CF_WRITEABLE | CF_HAS_KEY;
        }
        else
        {
            ACK_DEBUG_PRINT_E(
                "Append member: trying to append member %u of type %u, but already exists (as type %u)",
                key,
                type,
                foundType);
            return false;
        }
            
        return true;
    }

    // If we get here, we are actually appending. Context points at the insertion point.
    // Determine header length.
    insertionSize = 1;  // type byte, always exists.

    // Key exists if appending to an object and the key is not in range [0-15].
    encodedKey.ByteCount = 0;
    if ((DBF_NOT_IN_OBJECT == found) && ((0 == key) || (key > 15)))
    {
        EncodeInteger_(key, &encodedKey, false); // false -> don't flip bits, i.e. not negative
        insertionSize += encodedKey.ByteCount;
    }

    // Data.
    insertionSize += dataSize;

    // Grow the document block, and make a hole for the new member data.
    // Note that this does not change the location of the data.
    // Also if we're creating a new container, allocate space for the new entry in the container index.
    newEntrySize = ACK_IS_CONTAINER(type) ? sizeof(ContainerIndexEntry_t) : 0;
    if (NULL == HeapletSetSize(pParentContainer->Id, context.BlockSize + insertionSize + newEntrySize))
    {
        ACK_DEBUG_PRINT_E(
            "Append member: can't grow document %u, block size %u, delta %u",
            pParentContainer->Id,
            context.BlockSize,
            insertionSize + newEntrySize);
        return false;
    }

    pNewMember = (uint8_t*)context.BlockBase + context.Offset;
    memmove(pNewMember + insertionSize, pNewMember, context.BlockSize - context.Offset);

    if (DBF_NOT_IN_OBJECT == found)
    {
        // Appending to an object; store type byte, which may include the member key.
        if (key && (key <= 15))
        {
            *pNewMember++ = type | (key << 4);
        }
        else
        {
            *pNewMember++ = type;

            memcpy(pNewMember, encodedKey.Bytes, encodedKey.ByteCount);
            pNewMember += encodedKey.ByteCount;
        }
    }
    else
    {
        *pNewMember++ = type;
    }

    memcpy(pNewMember, pData, dataSize);

    // Update the container index. All entries with an offset > the insertion point need to be
    // adjusted upward in memory by the count of bytes we just inserted.
    entryCount = ((WriteableDocumentHeader_t*)context.BlockBase)->IndexEntryCount;
    for (i = 0; i < entryCount; ++i)
    {
        // The insertion point always points at an ACKDOC_ELEMENT_CONTAINER_END type byte/
        // Entries in the index always point at the header for a container, i.e. never
        // an ACKDOC_ELEMENT_CONTAINER_END, so the comparison here is > instead of >=.
        if (pIndex[i] > context.Offset)
        {
            pIndex[i] += insertionSize;
        }

        // All entries in the index need to shift if we're adding a new index entry.
        // Do this addition after the adjustment above for insertionSize.
        pIndex[i] += newEntrySize;
    }

    // Add new entry in the container index.
    if (ACK_IS_CONTAINER(type))
    {
        // Size of the move is the new size of the block, not including the new container index entry we
        // left room for, minus the document header and existing index entries.
        i = context.BlockSize
            + insertionSize
            - sizeof(WriteableDocumentHeader_t)
            - (pHeader->IndexEntryCount * sizeof(ContainerIndexEntry_t));

        memmove(pIndex + pHeader->IndexEntryCount + 1, pIndex + pHeader->IndexEntryCount, i);

        // The new container's heasder starts at the originally calculated insertion point
        // adjusted upwards by the size of the new container index entry we added.
        pIndex[pHeader->IndexEntryCount] = context.Offset + sizeof(ContainerIndexEntry_t);

        pContainerOut->Id = pParentContainer->Id;
        pContainerOut->Value = pHeader->IndexEntryCount;
        pContainerOut->Flags = CF_WRITEABLE | ((DBF_NOT_IN_OBJECT == found) ? CF_HAS_KEY : 0);

        ++pHeader->IndexEntryCount;
    }

    return true;
}

// If found, leaves context pointing at the member header for containers, or the member value for
// non-containers.
static bool AccessMember_(
    const ACKDocContainer_t* pParentContainer,
    Access_t access,
    unsigned targetMember,
    unsigned* pFoundMemberType,
    DocumentContext_t* pContext,
    Found_t* pFound)
{
    unsigned memberType;
    unsigned memberKey;
    unsigned memberHeaderOffset;
    unsigned memberIndex;
    bool containerIsObject;
    bool result;

    // Access the document, and load info about the container we're going to search for the
    // desired member.
    if (!AccessContainer_(pParentContainer, pContext, &containerIsObject))
    {
        ACK_DEBUG_PRINT_E("Access member: access document or load container failed");
        return false;
    }

    // Count is allowed only for arrays.
    if ((DBF_ACCESS_COUNT == access) && containerIsObject)
    {
        ACK_DEBUG_PRINT_E("Container is not an array; count not allowed");
        return false;
    }

    // When we get here, context points just past the container's header, which is where
    // its members start.
    
    memberIndex = 0;

next:
    memberHeaderOffset = pContext->Offset;

    if (!LoadMemberHeader_(pContext, &memberType, containerIsObject ? &memberKey : NULL))
    {
        ACK_DEBUG_PRINT_E("Access member: load member header failed");
        return false;
    }

    *pFoundMemberType = memberType;

    if (ACKDOC_ELEMENT_CONTAINER_END == memberType)
    {
        // Reached the end of the container and didn't find the member.
        // If we're counting members, return the count.
        if (DBF_ACCESS_COUNT == access)
        {
            *pFoundMemberType = memberIndex;
        }
    
        // Reset context to point at the member header.
        pContext->Offset = memberHeaderOffset;
        *pFound = containerIsObject ? DBF_NOT_IN_OBJECT : DBF_NOT_IN_ARRAY;

        return true;
    }

    if ((containerIsObject && (memberKey == targetMember))
        || (!containerIsObject && (memberIndex == targetMember) && (DBF_ACCESS_FIND == access)))
    {
        // Found it. Context points at the member value area, which is what we want except for
        // containers. For those we want to point at the header.
        if (ACK_IS_CONTAINER(memberType))
        {
            pContext->Offset = memberHeaderOffset;
        }

        *pFound = containerIsObject ? DBF_FOUND_IN_OBJECT : DBF_FOUND_IN_ARRAY;
        return true;
    }

    // This isn't the member we're looking for. Skip it.
    result = ACK_IS_CONTAINER(memberType)
        ? SkipContainerBody_(pContext, ACKDOC_ELEMENT_OBJECT == memberType)
        : SkipNonContainerMemberValue_(pContext, memberType);

    if (!result)
    {
        ACK_DEBUG_PRINT_E("Access member: can't skip member body/value");
        return false;
    }

    ++memberIndex;
    goto next;
}

// When this returns true, context points just past the container's header, which is where its members start.
static bool AccessContainer_(
    const ACKDocContainer_t* pContainer,
    DocumentContext_t* pContext,
    bool* pIsObject)
{
    size_t size;
    WriteableDocumentHeader_t* pHeader;
    unsigned containerType;
    unsigned containerKey;

    // Document.
    if (!HeapletGetBlock(pContainer->Id, &pContext->BlockBase, &size))
    {
        ACK_DEBUG_PRINT_E("Access container: no document in block %u", pContainer->Id);
        return false;
    }

    pContext->BlockSize = (unsigned)size;

    // The offset into the document could be directly given to us, or we might need to find it from
    // the container index.
    if (pContainer->Flags & CF_WRITEABLE)
    {
        pHeader = pContext->BlockBase;

        if (pContainer->Value >= pHeader->IndexEntryCount)
        {
            ACK_DEBUG_PRINT_E(
                "Access container: ordinal %u but only %u containers",
                pContainer->Value,
                pHeader->IndexEntryCount);
            return false;
        }

        pContext->Offset = ((const ContainerIndexEntry_t*)(pHeader + 1))[pContainer->Value];
    }
    else
    {
        pContext->Offset = pContainer->Value;
    }

    if (!LoadMemberHeader_(
        pContext,
        &containerType,
        (pContainer->Flags & CF_HAS_KEY) ? &containerKey : NULL)

        || !ACK_IS_CONTAINER(containerType))
    {
        ACK_DEBUG_PRINT_E("Access container: load container header failed, or not a container");
        return false;
    }

    *pIsObject = (ACKDOC_ELEMENT_OBJECT == containerType);
    return true;
}

// Leaves context pointing just past the header, i.e. at the member's value.
// If pKey is non-NULL, the member header has a key.
// If pKey is NULL, the member header has no key.
static bool LoadMemberHeader_(DocumentContext_t* pContext, unsigned* pType, unsigned* pKey)
{
    int type;

    // If we're at the end of the document, consider data-end to be the same as a container-end.
    // This is because trailing container-end can be omitted.
    *pType = ACKDOC_ELEMENT_CONTAINER_END;
    if (pContext->Offset == pContext->BlockSize)
    {
        return true;
    }

    // Type byte.
    type = LoadByte_(pContext);
    if (type < 0)
    {
        ACK_DEBUG_PRINT_E("Load member header: load type byte failed");
        return false;
    }

    *pType = (uint8_t)type;

    // Retrieve the key, if there's supposed to be one.
    // Container-end never has a key.
    if (pKey && (ACKDOC_ELEMENT_CONTAINER_END != type))
    {
        *pKey = *pType >> 4;
        if (0 == *pKey)
        {
            // Key stored separately.
            if (!LoadEncodedSmallUnsignedInteger_(pContext, pKey))
            {
                ACK_DEBUG_PRINT_E("Load member header: load key failed");
                return false;
            }
        }
        else
        {
            // Key stored in high nibble of type byte.
            *pType = (uint8_t)type & 0xf;
        }
    }

    return true;
}

// On input, context points at the first member in the container.
// On output, context points at the element after the container-end.
static bool SkipContainerBody_(DocumentContext_t* pContext, bool membersHaveKeys)
{
    unsigned memberType;
    unsigned memberKey;

next:
    if (!LoadMemberHeader_(pContext, &memberType, membersHaveKeys ? &memberKey : NULL))
    {
        ACK_DEBUG_PRINT_E("Skip container body: load member header failed");
        return false;
    }

    if (ACKDOC_ELEMENT_CONTAINER_END == memberType)
    {
        // Successfully reached end of container.
        return true;
    }

    if (ACK_IS_CONTAINER(memberType))
    {
        if (!SkipContainerBody_(pContext, ACKDOC_ELEMENT_OBJECT == memberType))
        {
            ACK_DEBUG_PRINT_E("Skip container body: skip nested container body failed");
            return false;
        }
    }
    else
    {
        if (!SkipNonContainerMemberValue_(pContext, memberType))
        {
            ACK_DEBUG_PRINT_E("Skip container body: skip member value failed");
            return false;
        }
    }

    goto next;
}

// On input, context points at the member value (not the header).
// On output, context points at the next element's header.
static bool SkipNonContainerMemberValue_(DocumentContext_t* pContext, unsigned memberType)
{
    bool result;

    union
    {
        int64_t Integer;
        double Double;
        float Float;
        const char* String;
    }
    dummyValue;

    switch (memberType)
    {
    case ACKDOC_ELEMENT_INTEGER:
    case ACKDOC_ELEMENT_NEGATIVE_INTEGER:
        // Negative or not doesn't matter for the purposes of skipping.
        result = LoadEncodedInteger_(pContext, &dummyValue.Integer, false);
        break;

    case ACKDOC_ELEMENT_FLOAT64:
        result = LoadFloat64_(pContext, &dummyValue.Double);
        break;

    case ACKDOC_ELEMENT_FLOAT32:
        result = LoadFloat32_(pContext, &dummyValue.Float);
        break;

    case ACKDOC_ELEMENT_STRING:
        result = LoadString_(pContext, &dummyValue.String);
        break;

    case ACKDOC_ELEMENT_TRUE:
    case ACKDOC_ELEMENT_FALSE:
    case ACKDOC_ELEMENT_NULL:
        // No-op.
        result = true;
        break;

    default:
        result = false;
        break;
    }

    if (!result)
    {
        ACK_DEBUG_PRINT_E("Skip member value: skip type %u failed", memberType);
    }

    return result;
}

static bool LoadEncodedInteger_(DocumentContext_t* pContext, int64_t* pValue, bool flipBits)
{
    uint64_t value;
    unsigned shiftCount;
    int byte;

    value = 0;
    shiftCount = 0;

    // Load using little-endian semantics.
    do
    {
        byte = LoadByte_(pContext);
        if (byte < 0)
        {
            ACK_DEBUG_PRINT_E("Load integer: load next byte failed");
            return false;
        }

        value |= (uint64_t)((unsigned)byte & 0x7f) << shiftCount;
        shiftCount += 7;
    }
    while (byte & 0x80);

    if (flipBits)
    {
        value = (int64_t)~(uint64_t)value;
    }

    *pValue = (int64_t)value;
    return true;
}

// Loads value in range [0-UINTMAX].
static bool LoadEncodedSmallUnsignedInteger_(DocumentContext_t* pContext, unsigned* pValue)
{
    int64_t value;

    if (!LoadEncodedInteger_(pContext, &value, false))  // false -> don't flip bits, i.e. not negative
    {
        ACK_DEBUG_PRINT_E("Load unsigned integer: load full integer failed");
        return false;
    }

    if ((value < 0) || (value > UINT_MAX))
    {
        ACK_DEBUG_PRINT_E("Load unsigned integer: %"PRId64" out of range for unsigned int", value);
        return false;
    }

    *pValue = (unsigned)(uint64_t)value;
    return true;
}

static void EncodeInteger_(int64_t value, EncodedInteger_t* pEncoded, bool flipBits)
{
    unsigned byte;

    if (flipBits)
    {
        value = (int64_t)~(uint64_t)value;
    }

    pEncoded->ByteCount = 0;

    // Store using little-endian semantics.
    do
    {
        byte = (unsigned)value & 0x7f;
        value = (int64_t)((uint64_t)value >> 7);

        if (value)
        {
            byte |= 0x80;
        }

        pEncoded->Bytes[pEncoded->ByteCount] = (uint8_t)byte;
        ++pEncoded->ByteCount;
    }
    while (value);
}

static bool LoadFloat64_(DocumentContext_t* pContext, double* pValue)
{
    ACK_STATIC_ASSERT(sizeof(*pValue) == 8);

    return LoadLittleEndian_(pContext, (uint8_t*)pValue, sizeof(*pValue));
}

static bool LoadFloat32_(DocumentContext_t* pContext, float* pValue)
{
    ACK_STATIC_ASSERT(sizeof(*pValue) == 4);

    return LoadLittleEndian_(pContext, (uint8_t*)pValue, sizeof(*pValue));
}

static bool LoadString_(DocumentContext_t* pContext, const char** ppValue)
{
    int byte;
    const char* pStart = (const char*)((uintptr_t)pContext->BlockBase + pContext->Offset);

    while ((byte = LoadByte_(pContext)) > 0)
    {
        // No-op.
    }

    if (byte)
    {
        ACK_DEBUG_PRINT_E("Load string: load next byte failed");
        return false;
    }

    *ppValue = pStart;
    return true;
}

static int LoadByte_(DocumentContext_t* pContext)
{
    int byte;

    if (pContext->Offset < pContext->BlockSize)
    {
        byte = *(uint8_t*)((uintptr_t)pContext->BlockBase + pContext->Offset);
        ++pContext->Offset;
    }
    else
    {
        ACK_DEBUG_PRINT_E(
            "Load byte: data exhausted at offset %u of %u",
            pContext->Offset,
            pContext->BlockSize);

        byte = -1;
    }

    return byte;
}

static bool LoadLittleEndian_(DocumentContext_t* pContext, uint8_t* pData, unsigned count)
{
    unsigned i;
    int byte;

    for (i = 0; i < count; ++i)
    {
        byte = LoadByte_(pContext);
        if (byte < 0)
        {
            ACK_DEBUG_PRINT_E("Load little-endian: load byte %u failed", i);
            return false;
        }

        *pData++ = (uint8_t)byte;
    }

    SwapOrderIfBigEndian_(pData - count, count);

    return true;
}

static void SwapOrderIfBigEndian_(uint8_t* pData, unsigned dataSize)
{
    uint8_t* p1;
    uint8_t* p2;
    uint8_t temp;

    // The compiler will probably optimize away IsBigEndian_ such that it boils down to a true/false known
    // at compile time, with only the correct block (if vs. else) actually emitted into the final binary.
    if (IsBigEndian_())
    {
        p1 = pData;
        p2 = pData + dataSize - 1;

        while (p1 < p2)
        {
            temp = *p1;
            *p1++ = *p2;
            *p2++ = temp;
        }
    }
}

// GCC seems to optimize this away for callers only when the caller is in the same translation unit.
// That's why this is a static function in here instead of a shared function in e.g. ack_core_common.c.
static bool IsBigEndian_(void)
{
    union
    {
        uint8_t Bytes[2];
        uint16_t Short;
    }
    value;

    ACK_STATIC_ASSERT(sizeof(value) == 2);

    value.Short = 0x0001;
    return 0 == value.Bytes[0];
}
