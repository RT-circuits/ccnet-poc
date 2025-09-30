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
extern uint8_t upstream_rx_flag;
extern uint8_t downstream_rx_flag;

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  UART receive complete callback (called from HAL interrupt)
  * @param  huart: UART handle
  * @retval None
  */
void UART_RxCpltCallback(UART_HandleTypeDef *huart);

/**
  * @brief  Process received bytes (call from main loop)
  * @retval None
  */
void UART_ProcessReceivedBytes(void);

/**
  * @brief  Initialize UART for message reception
  * @param  interface: Interface configuration
  * @param  message: Message structure to populate with received data
  * @retval None
  */
void UART_Init(interface_config_t* interface, message_t* message);

#ifdef __cplusplus
}
#endif

#endif /* __UART_H */