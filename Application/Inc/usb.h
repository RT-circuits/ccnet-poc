/**
  ******************************************************************************
  * @file           : usb.h
  * @brief          : USB Virtual COM Port header file
  *                   USB CDC communication functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __USB_H
#define __USB_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void USB_Init(void);
void USB_TransmitString(const char* str);
void USB_TransmitBytes(uint8_t* data, uint16_t length);
void USB_CDC_RxHandler(uint8_t* Buf, uint32_t Len);
uint8_t USB_IsInputReady(void);
uint8_t USB_GetInputLine(char* buffer, uint8_t max_length);
void USB_ProcessStatusMessage(void);
void USB_Tx(uint8_t* buffer, uint16_t length);
void USB_Flush(void);
void USB_CDCTransmitCpltHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_H */
