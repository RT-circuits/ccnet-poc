/**
  ******************************************************************************
  * @file           : proto_types.h
  * @brief          : Protocol types header file
  *                   Basic protocol type definitions to avoid circular dependencies
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __PROTO_TYPES_H
#define __PROTO_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  Protocol name enumeration
  */
typedef enum
{
    PROTO_CCNET = 0,
    PROTO_ID003,
    PROTO_CCTALK,
} proto_name_t;


#ifdef __cplusplus
}
#endif

#endif /* __PROTO_TYPES_H */
