/**
  ******************************************************************************
  * @file           : uart_test.h
  * @brief          : Simple UART test sequence header file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __UART_TEST_H
#define __UART_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

/* Exported functions prototypes ---------------------------------------------*/
void UART_RunAllTests(void);
void UART_TEST_E(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_TEST_H */
