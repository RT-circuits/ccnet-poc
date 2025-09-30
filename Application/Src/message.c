/**
  ******************************************************************************
  * @file           : message.c
  * @brief          : Message handling module implementation
  *                   Protocol message construction and management
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "message.h"
#include "crc.h"

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize message structure
  * @param  msg: pointer to message structure
  * @param  protocol: protocol type
  * @param  opcode: command/response opcode
  * @retval None
  */
void MESSAGE_Init(message_t* msg, proto_name_t protocol, uint8_t opcode)
{
    msg->protocol = protocol;
    msg->opcode = opcode;
    msg->data_length = 0;
    msg->length = 0;
    
    /* Clear data buffer */
    for (uint16_t i = 0; i < 250; i++)
    {
        msg->data[i] = 0;
    }
    
    /* Clear raw buffer */
    for (uint16_t i = 0; i < 256; i++)
    {
        msg->raw[i] = 0;
    }
}

/**
  * @brief  Set message data payload
  * @param  msg: pointer to message structure
  * @param  data: pointer to data buffer
  * @param  data_length: length of data buffer
  * @retval None
  */
void MESSAGE_SetData(message_t* msg, uint8_t* data, uint8_t data_length)
{
    if (data != NULL && data_length <= 250)
    {
        for (uint8_t i = 0; i < data_length; i++)
        {
            msg->data[i] = data[i];
        }
        msg->data_length = data_length;
    }
    else
    {
        msg->data_length = 0;
    }
}

/**
  * @brief  Construct complete message for transmission
  * @note   This function is used to construct a complete (raw) message for transmission.
  *         For both CCNET and ID003 protocols: header | length | opcode | data | crc
  *         For CCTALK protocol: TODO
  *         Can be used for GENERIC messages with or without CRC.
  * @param  msg: pointer to message structure
  *         required: msg->protocol, msg->opcode, msg->data_length
  *         optional: msg->data, msg->crc_enabled
  * @retval Updated msg->raw, msg->length
  */
void MESSAGE_Construct(message_t* msg)
{
    uint16_t pos = 0;
    uint16_t crc;
    uint8_t header_length;
    
    /* Set header bytes */
    switch (msg->protocol)
    {
        case PROTO_ID003:           
            /* Header byte */
            msg->raw[pos++] = 0xFC;
            header_length = 1;
        
            break;
            
        case PROTO_CCNET:
            /* Header bytes */
            msg->raw[pos++] = 0x02;
            msg->raw[pos++] = 0x03;
            header_length = 2;

            break;

        case PROTO_CCTALK:
            /* Header bytes */
            //TODO: Implement CCTALK header bytes

            break;
            
break;
}  

    /* Set length field */
    msg->raw[pos++] = header_length + 1 + 1 + msg->data_length + 2; /* header(1 or 2) + length + opcode + data + crc */
    
    /* Set opcode */
    msg->raw[pos++] = msg->opcode;
    
    /* Add data */
    if (msg->data_length > 0)
    {
        for (uint8_t i = 0; i < msg->data_length; i++)
        {
            msg->raw[pos++] = msg->data[i];
        }
    }

    /* Add CRC */
    pos = CRC_AppendCRC(msg, pos);
    msg->length = pos;
}

/* Private functions ---------------------------------------------------------*/
