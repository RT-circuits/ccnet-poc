/**
  ******************************************************************************
  * @file           : log.h
  * @brief          : Log module header file
  *                   USB-based logging system
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __LOG_H
#define __LOG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "message.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  Log level enumeration
  */
typedef enum
{
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_PROTO,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_VERBOSE
} log_level_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void LOG_Init(void);
void LOG_SetLevel(log_level_t level);
void LOG_Error(const char* message);
void LOG_Warn(const char* message);
void LOG_Proto(const message_t* msg);
void LOG_Info(const char* message);
void LOG_Debug(const char* message);
void LOG_Verbose(const char* message);
void LOG_Raw(const char* str);

#ifdef __cplusplus
}
#endif

#endif /* __LOG_H */
