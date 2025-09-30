/**
  ******************************************************************************
  * @file           : led.h
  * @brief          : LED driver header file
  *                   Contains LED control definitions and prototypes
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  LED state enumeration
  */
typedef enum
{
    LED_STATE_UNKNOWN = 0,
    LED_STATE_OFF,
    LED_STATE_ON,
    LED_STATE_FLASHING
} led_state_t;

/**
  * @brief  LED structure definition
  */
typedef struct
{
    GPIO_TypeDef* GPIO_Port;    /* GPIO port (e.g., GPIOA, GPIOB) */
    uint16_t GPIO_Pin;          /* GPIO pin (e.g., GPIO_PIN_0) */
    led_state_t state;          /* Current LED state */
} LED_HandleTypeDef;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* LED instances - defined in app.c */
extern LED_HandleTypeDef hled1;
extern LED_HandleTypeDef hled2;
extern LED_HandleTypeDef hled3;

/* Timer17 handle for LED flash timing - defined in main.c */
extern TIM_HandleTypeDef htim17;

/* Exported functions prototypes ---------------------------------------------*/
void LED_Init(void);
void LED_On(LED_HandleTypeDef* hled);
void LED_Off(LED_HandleTypeDef* hled);
void LED_Flash(LED_HandleTypeDef* hled, uint16_t time_ms);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* __LED_H */

