/**
  ******************************************************************************
  * @file           : btn.h
  * @brief          : Button handling header file
  *                   Config/Reset button functionality
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __BTN_H
#define __BTN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/* Config/Reset button state structure */
typedef struct {
    uint8_t press_detected;
    uint32_t press_start_time;
    uint8_t long_press_detected;
    uint8_t just_pressed;
    uint8_t just_released;
} config_reset_button_t;

/* Exported constants --------------------------------------------------------*/

/* Button handling constants */
#define CONFIG_RESET_BUTTON_PIN GPIO_PIN_8
#define CONFIG_RESET_BUTTON_PORT GPIOB
#define CONFIG_RESET_LONG_PRESS_MS 2000
#define CONFIG_RESET_LED_FLASH_MS 200

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void BTN_Init(void);
void BTN_ConfigResetButtonInterrupt(void);
void BTN_ProcessConfigResetButton(void);
uint8_t BTN_IsConfigMenuActive(void);

#ifdef __cplusplus
}
#endif

#endif /* __BTN_H */
