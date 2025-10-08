/**
  ******************************************************************************
  * @file           : crc.h
  * @brief          : CRC module header file
  *                   Contains CRC calculation definitions and prototypes
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __CRC_H
#define __CRC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "proto_types.h"

/* Forward declarations ------------------------------------------------------*/
typedef struct message_t message_t;

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  CRC type enumeration
  */
typedef enum
{
    CRC_OTHER = 0,
    CRC_CCNET,
    CRC_ID003,
    CRC_CCTALK
} crc_type_t;

/**
  * @brief  CRC type enumeration
  */
  typedef enum
  {
      CRC_NOT_OK = 0,
      CRC_OK,
  } crc_result_t;
/**
  * @brief  CRC configuration structure
  */
typedef struct
{
    crc_type_t type;           /* CRC type (OTHER/CRC_CCNET/CRC_CCTALK) */
    uint8_t bytesize;          /* CRC bytesize */
    uint32_t polynomial;       /* CRC polynomial */
    uint32_t start_value;      /* CRC start value */
} crc_config_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
uint16_t CRC_Calculate(uint8_t* data, proto_name_t protocol, uint16_t length);
uint16_t CRC_AppendCRC(message_t* msg, uint16_t pos);
crc_result_t CRC_Validate(message_t* msg);

#ifdef __cplusplus
}
#endif

#endif /* __CRC_H */
