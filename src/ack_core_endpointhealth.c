/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack.h"
#include "ack_core_common.h"
#include "ack_core_document_wrappers.h"
#include "ack_generated_ack.h"
#include "ack_generated_alexa_endpoint_health.h"
#include "ack_generated_connectivity.h"

ACKError_t EndpointHealthAddPropertyToEvent(const ACKStateCommon_t* pCommon)
{
    ACKDocContainer_t propertyObject;
    ACKDocContainer_t propertyValueObject;
    bool result;

    ACK_REENTRY_CHECK;

    result = AddPropertyToEventDocument(
        pCommon,
        ack_namespace_alexa_endpoint_health,
        NULL, // No instance for this capability
        ack_alexa_endpoint_health_property_name_connectivity,
        &propertyObject)

        && ACKDoc_AppendObject(&propertyObject, ack_property_object_member_value, &propertyValueObject)
        
        && ACKDoc_AppendInt(
            &propertyValueObject,
            ack_alexa_endpoint_health_connectivity_property_value_object_member_value,
            ack_connectivity_ok);

    return result ? ACK_NO_ERROR : ACK_ERR_OUT_OF_MEMORY;
}
