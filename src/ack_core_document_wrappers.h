/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#ifndef __ACK_CORE_DOCUMENT_WRAPPERS_H__
#define __ACK_CORE_DOCUMENT_WRAPPERS_H__

#include "ack_document.h"
#include "ack_generated_ack.h"
#include "V3AVS.pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

// This is the single namespace in the AVS "envelope" for all DOM Binary Form messages, in both directions
// (directives and events). This is a value in V3AVS_Namespace enum in V3AVS.proto.
#define ACK_DOM_BINARY_FORM_ENVELOPE_NAMESPACE V3AVS_Namespace_ACK_DOM_BINARY_FORM

typedef struct _ACKEventDocument_t
{
    ACKDocContainer_t PayloadObject;
    ACKDocContainer_t ContextObject;
}
ACKEventDocument_t;

extern ACKEventDocument_t g_eventDocument;
#ifdef ACK_COMPOUND_DEVICE
extern uint16_t g_eventEndpointId;
#endif

bool CreateEventDocument(ack_namespace eventNamespace, unsigned eventName, unsigned flags);

#define CEDF_OMIT_CONTEXT 0x0001

bool AddPropertyToEventDocument(
    const ACKStateCommon_t* pCommon,
    ack_namespace v3Namespace,
    const uint32_t* pInstance,
    unsigned name,
    ACKDocContainer_t* pPropertyObject);    // returned to caller, for adding actual value

#ifdef __cplusplus
}
#endif

#endif // ndef __ACK_CORE_DOCUMENT_WRAPPERS_H__
