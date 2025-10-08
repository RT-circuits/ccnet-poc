/**
  ******************************************************************************
  * @file           : app.h
  * @brief          : Application layer header file
  *                   Contains application-specific definitions and prototypes
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __APP_H
#define __APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "proto_types.h"
#include "message.h"
#include "proto.h"

/* Exported types ------------------------------------------------------------*/


/**
  * @brief  Physical interface polarity enumeration
  */
typedef enum
{
    POLARITY_NORMAL = 0,    /* Normal signal polarity */
    POLARITY_INVERTED       /* Inverted signal polarity */
} polarity_t;

/**
  * @brief  Protocol role enumeration
  */
typedef enum
{
    ROLE_BILL_VALIDATOR = 0,    /* Bill validator role */
    ROLE_CONTROLLER             /* Controller role */
} proto_role_t;


/**
  * @brief  Physical interface configuration structure
  * @note   Contains UART hardware communication parameters
  */
typedef struct
{
    uint32_t baudrate;              /* Communication baudrate (e.g., 9600, 115200) */
    unsigned long parity;           /* Parity setting (e.g., UART_PARITY_NONE) */
    polarity_t uart_polarity;       /* Signal polarity (normal/inverted) */
    UART_HandleTypeDef* uart_handle; /* Reference to UART hardware handle */
} phy_config_t;

/**
  * @brief  Datalink layer configuration structure
  * @note   Contains protocol-specific datalink parameters
  */
typedef struct
{
    uint32_t polling_period_ms;     /* Polling period in milliseconds */
    uint8_t sync_length;            /* Number of sync bytes */
    uint8_t sync_byte1;             /* First sync byte */
    uint8_t sync_byte2;             /* Second sync byte (if sync_length > 1) */
    int8_t length_offset;           /* Length field offset (0 for CCNET/ID003, -5 for CCTALK) */
    uint8_t crc_length;             /* Number of CRC bytes */
    uint32_t inter_byte_timeout_ms; /* Inter-byte timeout in milliseconds */
} datalink_config_t;



/**
  * @brief  Interface configuration structure
  * @note   Complete configuration for a communication interface
  */
typedef struct
{
    proto_name_t protocol;         /* Protocol type (PROTO_CCNET/CCTALK/ID003) */
    proto_role_t role;             /* Device role (ROLE_BILL_VALIDATOR/CONTROLLER) */
    phy_config_t phy;              /* Physical layer configuration */
    datalink_config_t datalink;    /* Datalink layer configuration */
} interface_config_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* UART handles - defined in main.c */
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

/* Interface objects */
extern interface_config_t if_upstream;    /* Upstream interface (CCNET) */
extern interface_config_t if_downstream;  /* Downstream interface (ID003) */

/* Message objects */
extern message_t upstream_msg;    /* CCNET messages from upstream */
extern message_t downstream_msg;  /* ID003 messages from downstream */

/* Exported functions prototypes ---------------------------------------------*/
void APP_Init(void);
void APP_Process(void);
void APP_MCUReset(void);
void APP_ShowConfigMenu(void);
message_parse_result_t APP_CheckForDownstreamMessage(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_H */

