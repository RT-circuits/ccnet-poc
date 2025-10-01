/**
  ******************************************************************************
  * @file           : message.h
  * @brief          : Message handling module header file
  *                   Protocol message structure and construction functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __MESSAGE_H
#define __MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "proto.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  Message direction enumeration
  */
typedef enum
{
    MSG_DIR_TX = 0,    /* Transmit message (outgoing) */
    MSG_DIR_RX         /* Receive message (incoming) */
} message_direction_t;

/**
  * @brief  Message parsing result enumeration
  */
typedef enum
{
    MSG_OK = 0,
    MSG_UNKNOWN_OPCODE,
    MSG_DATA_MISSING_FOR_OPCODE,
    MSG_CRC_WRONG,
    MSG_INVALID_LENGTH,
    MSG_INVALID_HEADER,
    MSG_PARSE_ERROR
} message_result_t;

/**
  * @brief  Message structure for protocol handling
  */
typedef struct {
    proto_name_t protocol;        /* Protocol type (PROTO_ID003, PROTO_CCTALK, PROTO_CCNET) */
    message_direction_t direction; /* Message direction (TX/RX) */
    uint8_t opcode;              /* Command/response opcode */
    uint8_t data[250];           /* Payload data buffer */
    uint8_t data_length;         /* Length of data payload */
    uint8_t raw[256];            /* Complete message bytes ready to transmit */
    uint8_t length;             /* Total message length in raw buffer */
} message_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void MESSAGE_Construct(message_t* msg);
void MESSAGE_Init(message_t* msg, proto_name_t protocol, message_direction_t direction, uint8_t opcode);
void MESSAGE_SetData(message_t* msg, uint8_t* data, uint8_t data_length);
message_result_t MESSAGE_Parse(message_t* msg, uint8_t* raw_data, uint16_t raw_length);
const char* MESSAGE_GetOpcodeASCII(proto_name_t protocol, message_direction_t direction, uint8_t opcode);
message_result_t MESSAGE_ValidateOpcode(proto_name_t protocol, message_direction_t direction, uint8_t opcode);

#ifdef __cplusplus
}
#endif

#endif /* __MESSAGE_H */
