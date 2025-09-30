/**
  ******************************************************************************
  * @file           : config.h
  * @brief          : Configuration module header file
  *                   Configuration management and CLI interface
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  Configuration settings structure
  */
typedef struct
{
    interface_config_t upstream;      /* Upstream interface configuration */
    interface_config_t downstream;    /* Downstream interface configuration */
    uint8_t usb_logging_enabled;     /* USB logging enabled flag */
    uint8_t protocol_logging_verbose; /* Protocol logging verbosity */
    uint8_t bill_table[8];           /* Bill table mapping (8 bits) */
} config_settings_t;

/* Exported constants --------------------------------------------------------*/

/* Configuration menu options */
#define CONFIG_MENU_UPSTREAM_PROTOCOL    1
#define CONFIG_MENU_UPSTREAM_BAUDRATE    2
#define CONFIG_MENU_UPSTREAM_PARITY      3
#define CONFIG_MENU_DOWNSTREAM_PROTOCOL  4
#define CONFIG_MENU_DOWNSTREAM_BAUDRATE 5
#define CONFIG_MENU_DOWNSTREAM_PARITY   6
#define CONFIG_MENU_DOWNSTREAM_POLLING  7
#define CONFIG_MENU_BILL_TABLE          8
#define CONFIG_MENU_USB_LOGGING         9
#define CONFIG_MENU_PROTOCOL_LOGGING    10
#define CONFIG_MENU_EXIT                11

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Global configuration settings */
extern config_settings_t g_config;

/* Exported functions prototypes ---------------------------------------------*/
void CONFIG_Init(void);
void CONFIG_LoadFromNVM(void);
void CONFIG_SaveToNVM(void);
void CONFIG_ShowMenu(void);
void CONFIG_ProcessMenu(void);
void CONFIG_DisplayCurrentSettings(void);
void CONFIG_DisplayInterfaceSettings(const char* name, interface_config_t* interface);
void CONFIG_UpdateUpstreamProtocol(void);
void CONFIG_UpdateUpstreamBaudrate(void);
void CONFIG_UpdateUpstreamParity(void);
void CONFIG_UpdateDownstreamProtocol(void);
void CONFIG_UpdateDownstreamBaudrate(void);
void CONFIG_UpdateDownstreamParity(void);
void CONFIG_UpdateDownstreamPolling(void);
void CONFIG_UpdateBillTable(void);
void CONFIG_UpdateUsbLogging(void);
void CONFIG_UpdateProtocolLogging(void);
void CONFIG_DisplayBaudrateOptions(void);
void CONFIG_DisplayParityOptions(void);
void CONFIG_DisplayProtocolOptions(void);
void CONFIG_DisplayPollingOptions(void);
void CONFIG_DisplayBillTableBinary(void);
void CONFIG_DisplayProtocol(uint8_t protocol);
void CONFIG_DisplayBaudrate(uint32_t baudrate);
void CONFIG_DisplayParity(uint32_t parity);
void CONFIG_FlushBuffer(void);
void CONFIG_BufferWrite(const char* str);

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H */
