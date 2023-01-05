/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

/*
    Heaplet: a dynamic memory manager operating over a fixed buffer.

    Every possible allocated block has a well-known, unique name in the form of an integral tag. All
    operations target a specific block, by tag. Blocks are automatically moved around the buffer such that
    all allocated blocks are kept contiguous at all times; there is no fragmentation in a heaplet. Accessing
    an allocated block therefore requires an additional operation, to get a current pointer to it.
    Unallocated blocks consume no space in the heaplet.

    Tag values are significant in that blocks are kept sorted; blocks with lower tags are at lower addresses
    in memory than blocks with higher tags. This guarantees that when a block with a higher tag is resized,
    blocks with a lower tag will not move. Used judiciously, this arrangement can minimize the impact of the
    automatic memory contiguity in the heaplet, and for example allow a piece of code to build data structures
    inside blocks with higher tags that contain pointers to data in blocks with lower tags.

    Note that 0 is a valid size for allocated blocks. This avoids workarounds in client code when a block size
    is calculated as 0.

    Supported operations on the heaplet are
    - Set the size of a block, which encompasses both allocating blocks and changing the size of blocks
      already allocated. This operation causes movement of all allocated blocks with tags higher than
      that of the subject block.
    - Free a block. This operation causes movement of all remaining allocated blocks with tags higher than
      that of the subject block.
    - Access a block. Returns a pointer to the block's current address. It is generally unsafe to cache
      pointers from this operation across set-size and free operations except as described above for those
      operations.
    - Get the current size of a block. This helps eliminate the need to track sizes in client code.

    Allocated blocks are kept aligned in memory such that they are suitable to contain any kind of data
    or structures.

    The design of the heaplet deliberately trades away speed for the ability to maximize memory efficiency.
    Keeping allocated blocks contiguous in memory, and the extra level of indirection required in order to
    access blocks in the heaplet, are relatively expensive features. However minimizing memory use is more
    important than the processing efficiency lost maintaining the heaplet.

    The heaplet is a singleton. Functions declared in this header file operate on the single instance.
 */

#ifndef __ACK_HEAPLET_H__
#define __ACK_HEAPLET_H__

#include "ack_user_config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ACK_ENABLE_ASSERT
#define ACK_MEMORY_DIAG
#endif

// The type for a memory block tag.
typedef uint8_t HeapletTag_t;
#define MEM_TAG_MAX 255

// Initializes or reinitializes the heaplet singleton.
void HeapletInit(void);

// Allocates or resizes a block.
// If the block is currently allocated, it is resized.
// If the block is currently unallocated, it is allocated.
// Note that zero is a valid size for an allocated block, and a block with zero size is distinct from an
// unallocated block.
// Returns a pointer to the block, or NULL if there's not enough space to alloc/resize.
// Cached pointers to blocks with a higher tag than the block being allocated/resized are invalid after
// calling this routine, unless NULL is returned.
void* HeapletSetSize(HeapletTag_t tag, size_t newSize);

// Frees a block.
// Cached pointers to blocks with a higher tag than the block being freed are invalid after calling
// this routine, unless the block being freed is not currently allocated.
void HeapletFree(HeapletTag_t tag);

// Returns the size of a block. The returned size is the size as most recently specified to
// HeapletSetSize (not the aligned size).
// This routine cannot be used to distinguish between allocated and unallocated blocks. If zero is returned,
// the block is either unallocated, or allocated at 0 size.
size_t HeapletGetSize(HeapletTag_t tag);

// Returns the current address of an allocated block.
// Exercise care when caching, as block addresses can change as blocks are allocated/resized and freed.
// A return value of NULL means the block is currently unallocated.
void* HeapletAccess(HeapletTag_t tag);

bool HeapletGetBlock(HeapletTag_t tag, void** ppBlock, size_t* pSize);

// Sets the size of a block to the maximum possible size, e.g. causes a block to be allocated such that
// all space in the heaplet is exhausted.
void* HeapletSetToLargestPossibleSize(HeapletTag_t tag);

#ifdef ACK_MEMORY_DIAG

// Returns tags of blocks currently allocated.
// maxTags is the size in tags of the buffer pointed to by pTags.
// Returns the actual tag count.
// The returned list will end with MEM_NONE, making it suitable to pass to HeapletAssertBlocksInUse.
size_t HeapletGetBlocksInUse(HeapletTag_t* pTags, size_t maxTags);

// Asserts that the set of currently allocated blocks matches a specific list.
void HeapletAssertBlocksInUse(const HeapletTag_t* pExpectedTags);

// Ensures that the next call to HeapletSetSize that attempts to grow a particular block will fail.
// Only one pending failure is allowed; calling this function replaces any pending failure tag with the one
// provided. Passing MEM_NONE removes any previously set pending failure tag.
void HeapletInjectSetSizeFailure(HeapletTag_t tag);

#endif

// Memory tags.
// The heaplet is always maintained such that blocks with lower tags are lower in memory than blocks
// with higher tags.

// No tag. May not be used.
#define MEM_NONE 0

// Usage for requesting next (inbound) event is as follows (a=alloc/realloc, f=free).
// ----------------------------------------------------------------------------------
// Request next event:
// a MEM_RECEIVED_ENCODED_ACP_RESPONSE          encoded acp_response
//
// Process directive (when next event was directive):
// f MEM_RECEIVED_ENCODED_ACP_RESPONSE          have payload; encoded response not needed
// a MEM_RECEIVED_DIRECTIVE_INFO                info from V3AVS_AvsDirective acp_response
// a MEM_RECEIVED_BINARY_DOCUMENT
//   (--- call ACKUser_OnXxx handler --- )
// f MEM_RECEIVED_DIRECTIVE_INFO
// f MEM_RECEIVED_BINARY_DOCUMENT
//
//
// Usage for reading OTA chunk and similar request/response routines is as follows.
// --------------------------------------------------------------------------------
// a MEM_RECEIVED_ENCODED_ACP_RESPONSE          encoded acp_response
// f MEM_RECEIVED_ENCODED_ACP_RESPONSE
//
//
// Usage for sending outbound events is as follows.
// ------------------------------------------------
// StartEvent:
// a MEM_OUTBOUND_BINARY_DOCUMENT               DOM Binary Form document
//
// Send:
// a MEM_OUTBOUND_ENCODED_ACP_CMD               encoded acp_cmd
// f MEM_OUTBOUND_ENCODED_ACP_CMD
// f MEM_OUTBOUND_BINARY_DOCUMENT

#define MEM_RECEIVED_ENCODED_ACP_RESPONSE 10
#define MEM_RECEIVED_DIRECTIVE_INFO 20

// Memory for logging and metrics data. Logging and metrics data are sent directly within one function.
// This tag is used to send logging and metrics without disturbing or conflicting with any other blocks
// which might be in use. This allows using logging and metrics without the re-entry restriction, e.g.
// from handler callbacks when processing directives or other inbound events (ACKUser_Onxxx).
#define MEM_OUTBOUND_LOG_AND_METRICS_DATA 90

// DOM Binary Form event document.
#define MEM_OUTBOUND_BINARY_DOCUMENT 120
#define MEM_OUTBOUND_ENCODED_ACP_CMD 130

// DOM Binary Form directive document.
#define MEM_RECEIVED_BINARY_DOCUMENT 230

#ifdef __cplusplus
}
#endif

#endif // def __ACK_HEAPLET_H__
