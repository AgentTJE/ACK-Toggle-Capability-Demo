/*
 * Copyright 2018-2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * You may not use this file except in compliance with the terms and conditions set forth in the
 * accompanying LICENSE.TXT file. THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
 * DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 */

#include "ack_core_communication.h"
#include "ack_core_communication_protocol.h"
#include "ack_core_heaplet.h"
#include "ack_core_power_saving.h"
#include "ack_core_protobuf_utilities.h"
#include "ack_core_validation.h"
#include "ack_user_platform.h"
#include "acp.pb.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ACKError_t ReadAck_(void)
{
    AcpAck_t ack;
    ACK_DEBUG_PRINT_D("Reading ACK message.");

    if (!ACKPlatform_Receive(&ack, sizeof(ack), ACP_IO_RESPONSE_TIMEOUT_MS))
    {
        ACK_DEBUG_PRINT_W("I/O error reading ack.");
        VALIDATION_SEND_MARKER(ACK_MARKER_PROTOCOL_ERROR, NULL);

        return ACK_ERR_IO;
    }

    ACK_DEBUG_PRINT_D("ACK value: %u.", ack.Value);

    if (ACK_ACP_NO_ERROR != (AcpValue_t)ack.Value)
    {
        ACK_DEBUG_PRINT_E("Alexa Connect Kit returned failure ack: %u.", ack.Value);
        VALIDATION_SEND_MARKER(ACK_MARKER_PROTOCOL_ERROR, NULL);

        return ACK_ERR_PROTOCOL;
    }

    return ACK_NO_ERROR;
}

static ACKError_t SendRequestPacket_(
    uint8_t type,
    uint32_t crc32,
    size_t dataLength)
{
    ACK_DEBUG_PRINT_D("Sending request header.");

    AcpRequest_t req;
    ACKError_t ret = ACK_NO_ERROR;
    uint8_t header[ACP_REQUEST_HDR_LEN] = ACP_REQUEST_HDR_BYTES;
    unsigned retries = 0;

    ACK_STATIC_ASSERT(sizeof(req.Header) == sizeof(header));
    memcpy(req.Header, header, sizeof(req.Header));

    req.Type = type;
    req.Version[0] = ACP_PROTOCOL_MAJOR_VERSION;
    req.Version[1] = ACP_PROTOCOL_MINOR_VERSION;
    req.Length = dataLength;
    req.Crc32 = crc32;

    do
    {
        retries++;

        // Clear the UART buffer in case during these retries there is stale data
        ACKPlatform_DrainRead();

        if (!ACKPlatform_Send(&req, sizeof(req), ACP_IO_DEFAULT_TIMEOUT_MS))
        {
            ACK_DEBUG_PRINT_E("I/O error writing request header.");
            VALIDATION_SEND_MARKER(ACK_MARKER_PROTOCOL_ERROR, NULL);

            return ACK_ERR_IO;
        }

        ret = ReadAck_();

        if (ACK_ERR_IO == ret)
        {
            ACK_DEBUG_PRINT_I("Timeout waiting for Ack. Retrying sending request header.");

            continue;
        }

        if (ACK_NO_ERROR != ret)
        {
            ACK_DEBUG_PRINT_E("Error %u receiving ACK for request header.", ret);

            return ret;
        }
    } while ((ACK_NO_ERROR != ret) && (retries < ACK_IO_MAX_RETRIES));

    if (ACK_NO_ERROR == ret)
    {
        ACK_DEBUG_PRINT_D("Completed sending request header for type %u.", type);

        return ACK_NO_ERROR;
    }

    if ((ACK_IO_MAX_RETRIES > 1) && (retries >= ACK_IO_MAX_RETRIES))
    {
      ACK_DEBUG_PRINT_E("Stop retrying after timeouts for type %u.", type);
    }

      return ret;
}

static ACKError_t SendProtobufPacket_(const void* pBuffer, size_t length)
{
    ACK_DEBUG_PRINT_D("Sending request body of length %u.", (unsigned)length);

    ACKError_t ret = ACK_NO_ERROR;

    if (!ACKPlatform_Send(pBuffer, length, ACP_IO_DEFAULT_TIMEOUT_MS))
    {
        ACK_DEBUG_PRINT_E("I/O error sending request body.");
        VALIDATION_SEND_MARKER(ACK_MARKER_PROTOCOL_ERROR, NULL);

        return ACK_ERR_IO;
    }

    ret = ReadAck_();
    if (ACK_NO_ERROR != ret)
    {
        ACK_DEBUG_PRINT_E("Error %u receiving ACK for request body.", ret);
        return ret;
    }

    return ACK_NO_ERROR;
}

ACKError_t CommsSendAcpProtobufCmd(const void* pBuffer, size_t length)
{
    // Acquire a short hold lock
    LOCK_POWER_SAVING();

    ACK_DEBUG_PRINT_D("Sending protobuf command, length %u.", (unsigned)length);

    uint32_t crc = ACKPlatform_CalculateCrc32(pBuffer, length);
    ACK_DEBUG_PRINT_D("Calculated crc %x.", crc);

    ACKError_t retval = SendRequestPacket_(ACP_REQUEST_WRITE_CMD, crc, length);
    if (ACK_NO_ERROR != retval)
    {
        return retval;
    }

    return SendProtobufPacket_(pBuffer, length);
}

ACKError_t CommsGetAcpResponse(void)
{
    // Acquire a short hold lock
    LOCK_POWER_SAVING();

    ACKError_t retval = ACK_NO_ERROR;
    AcpResponseHeader_t responseHeader;
    void* pBuffer;
    uint32_t crc;

    ACK_DEBUG_PRINT_D("Getting acp response headers.");

    retval = SendRequestPacket_(ACP_REQUEST_READ_RESPONSE_HEADER, 0, 0);
    if (retval != ACK_NO_ERROR)
    {
        goto cleanup0;
    }

    if (!ACKPlatform_Receive(&responseHeader, sizeof(responseHeader), ACP_IO_RESPONSE_TIMEOUT_MS))
    {
        ACK_DEBUG_PRINT_E("I/O error reading response header from connectivity module.");
        VALIDATION_SEND_MARKER(ACK_MARKER_PROTOCOL_ERROR, NULL);

        retval = ACK_ERR_IO;
        goto cleanup0;
    }

    ACK_DEBUG_PRINT_D("Getting acp response body, length is %"PRIu32".", responseHeader.Length);

    retval = SendRequestPacket_(ACP_REQUEST_READ_RESPONSE_BODY, 0, 0);
    if (retval != ACK_NO_ERROR)
    {
        goto cleanup0;
    }

    // Allocate memory and read into it.
    pBuffer = HeapletSetSize(MEM_RECEIVED_ENCODED_ACP_RESPONSE, responseHeader.Length);
    if (NULL == pBuffer)
    {
        retval = ACK_ERR_OUT_OF_MEMORY;
        goto cleanup0;
    }

    if (!ACKPlatform_Receive(pBuffer, responseHeader.Length, ACP_IO_RESPONSE_TIMEOUT_MS))
    {
        ACK_DEBUG_PRINT_E("I/O error reading response.");
        VALIDATION_SEND_MARKER(ACK_MARKER_PROTOCOL_ERROR, NULL);

        retval = ACK_ERR_IO;
        goto cleanup0;
    }

    // Check CRC32
    crc = ACKPlatform_CalculateCrc32(pBuffer, responseHeader.Length);

    if (crc != responseHeader.Crc32)
    {
        ACK_DEBUG_PRINT_E(
            "Incorrect CRC in received response. Received %"PRIx32" expected %"PRIx32".",
            crc,
            responseHeader.Crc32);

        retval = ACK_ERR_PROTOCOL;
        goto cleanup0;
    }

cleanup0:
    if (ACK_NO_ERROR != retval)
    {
        HeapletFree(MEM_RECEIVED_ENCODED_ACP_RESPONSE);
    }

    return retval;
}

