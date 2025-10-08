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
#include "proto_types.h"

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
    MSG_NO_MESSAGE = 0,
    MSG_OK,
    MSG_UNKNOWN_OPCODE,
    MSG_DATA_MISSING_FOR_OPCODE,
    MSG_CRC_INVALID,
    MSG_INVALID_LENGTH,
    MSG_INVALID_HEADER,
    MSG_PARSE_ERROR
} message_parse_result_t;

/**
  * @brief  Message structure for protocol handling
  */
typedef struct message_t {
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
void MESSAGE_Init(message_t* msg, proto_name_t protocol, message_direction_t direction);
message_t MESSAGE_Create(proto_name_t protocol, message_direction_t direction, uint8_t opcode, uint8_t* data, uint8_t data_length);
message_parse_result_t MESSAGE_Parse(message_t* msg);
const char* MESSAGE_GetOpcodeASCII(const message_t* msg);
message_parse_result_t MESSAGE_ValidateOpcode(message_t* msg);

#ifdef __cplusplus
}
#endif

#endif /* __MESSAGE_H */
