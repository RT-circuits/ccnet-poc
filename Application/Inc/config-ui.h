/**
  ******************************************************************************
  * @file           : config-ui.h
  * @brief          : Configuration UI module header
  *                   Menu interface and user interaction for configuration
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __CONFIG_UI_H__
#define __CONFIG_UI_H__

/* Includes ------------------------------------------------------------------*/
#include "config.h"

/* Menu constants ------------------------------------------------------------*/
#define CONFIGUI_MENU_UPSTREAM_PROTOCOL      1
#define CONFIGUI_MENU_UPSTREAM_BAUDRATE      2
#define CONFIGUI_MENU_UPSTREAM_PARITY        3
#define CONFIGUI_MENU_DOWNSTREAM_PROTOCOL    4
#define CONFIGUI_MENU_CCTALK_ADDRESSES       5
#define CONFIGUI_MENU_DOWNSTREAM_BAUDRATE    6
#define CONFIGUI_MENU_DOWNSTREAM_PARITY      7
#define CONFIGUI_MENU_DOWNSTREAM_POLLING     8
#define CONFIGUI_MENU_SHOW_BILL_TABLE        9
#define CONFIGUI_MENU_USB_LOGGING            10
#define CONFIGUI_MENU_LOG_LEVEL              11
#define CONFIGUI_MENU_EXIT                   12
#define CONFIGUI_MENU_SAVE_EXIT              13

/* Exported function prototypes ----------------------------------------------*/
void CONFIGUI_ShowMenu(void);
void CONFIGUI_ProcessMenu(void);
void CONFIGUI_ShowConfiguration(void);

#endif /* __CONFIG_UI_H__ */
