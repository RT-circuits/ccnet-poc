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
  * @brief  Message structure for protocol handling
  */
typedef struct {
    proto_name_t protocol;    /* Protocol type (PROTO_ID003, PROTO_CCTALK, PROTO_CCNET) */
    uint8_t opcode;          /* Command/response opcode */
    uint8_t data[250];       /* Payload data buffer */
    uint8_t data_length;     /* Length of data payload */
    uint8_t raw[256];        /* Complete message bytes ready to transmit */
    uint8_t length;         /* Total message length in raw buffer */
} message_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void MESSAGE_Construct(message_t* msg);
void MESSAGE_Init(message_t* msg, proto_name_t protocol, uint8_t opcode);
void MESSAGE_SetData(message_t* msg, uint8_t* data, uint8_t data_length);

#ifdef __cplusplus
}
#endif

#endif /* __MESSAGE_H */
