/**
  ******************************************************************************
  * @file           : uart.h
  * @brief          : Simple UART driver header file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __UART_H
#define __UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app.h"
#include "message.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern uint8_t downstream_rx_flag;

/* Exported functions prototypes ---------------------------------------------*/
void UART_RxCpltCallback(UART_HandleTypeDef *huart);
uint8_t UART_CheckForUpstreamData(void);
uint8_t UART_CheckForDownstreamData(void);
void UART_Init(interface_config_t* interface, message_t* message);
void UART_TransmitMessage(interface_config_t* interface, message_t* message);

#ifdef __cplusplus
}
#endif

#endif /* __UART_H */