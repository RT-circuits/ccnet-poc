/**
  ******************************************************************************
  * @file           : led.c
  * @brief          : LED driver implementation
  *                   LED control functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "led.h"
#include "tim.h"

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void LED_TimerCallback(void);
static void LED_AllOff(void);
static void LED_InitTimer(void);

/* Exported functions --------------------------------------------------------*/

void LED_Init(void)
{ 
  LED_AllOff();
  LED_InitTimer();

  /* test the LEDs */
  uint32_t delay = 500;
  LED_On(&hled1);
  HAL_Delay(delay);
  LED_Off(&hled1);
  LED_On(&hled2);
  HAL_Delay(delay);
  LED_Off(&hled2);
  LED_On(&hled3);
  HAL_Delay(delay);
  LED_Off(&hled3);
  LED_Flash(&hled1, 200);
}

/**
  * @brief  Flash LED for specified time using Timer17
  * @param  hled: pointer to LED handle
  * @param  duration_ms: time to keep LED on in milliseconds
  * @retval None
  */
  void LED_Flash(LED_HandleTypeDef* hled, uint16_t duration_ms)
  {
      
      /* Set LED state to flashing */
      hled->state = LED_STATE_FLASHING;
      
      /* Turn LED on */
      HAL_GPIO_WritePin(hled->GPIO_Port, hled->GPIO_Pin, GPIO_PIN_SET);
      
      /* Start timer with duration and LED callback */
      TIM_StartWithDuration(duration_ms, LED_TimerCallback);
  }
  
  /**
    * @brief  Initialize Timer17 for LED flashing
    * @retval None
    */
  static void LED_InitTimer(void)
  {
    /* Initialize Timer17 for LED flashing */
    htim17.Init.Period = 100 - 1;  /* Default 100us period */
    htim17.Init.Prescaler = (SystemCoreClock / 10000) - 1;  /* 10kHz */
    htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim17.Init.RepetitionCounter = 0;
    htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_Base_Init(&htim17) != HAL_OK)
    {
        Error_Handler();
    }
  }
  
  /**
    * @brief  LED timer callback - called when LED flash duration expires
    * @retval None
    */
  static void LED_TimerCallback(void)
  {
      /* Turn off all LEDs for robustness */
      LED_AllOff();
  }
  

void LED_On(LED_HandleTypeDef* hled)
{
    /* Turn on LED */
    HAL_GPIO_WritePin(hled->GPIO_Port, hled->GPIO_Pin, GPIO_PIN_SET);
    hled->state = LED_STATE_ON;
}

/**
  * @brief  Turn off LED
  * @param  hled: pointer to LED handle
  * @retval None
  */
void LED_Off(LED_HandleTypeDef* hled)
{
    /* Turn off LED */
    HAL_GPIO_WritePin(hled->GPIO_Port, hled->GPIO_Pin, GPIO_PIN_RESET);
    hled->state = LED_STATE_OFF;
}

/**
  * @brief  Turn off all LEDs
  * @retval None
  */
static void LED_AllOff(void)
{
    LED_Off(&hled1);
    LED_Off(&hled2);
    LED_Off(&hled3);
}

