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
extern TIM_HandleTypeDef htim16;  /* Timer16 handle from main.c */
extern TIM_HandleTypeDef htim17;  /* Timer17 handle from main.c */

/* Private function prototypes -----------------------------------------------*/
static void LED_TimerCallback(void);
static void LED_Timer16Callback(void);
static void LED_Timer17Callback(void);
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
  * @brief  Flash LED for specified time using Timer16 or Timer17
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
      
      /* Start appropriate timer based on LED */
      if (hled == &hled1)
      {
          /* Use Timer16 for LED1 */
          TIM_StartWithDuration_Timer16(duration_ms, LED_Timer16Callback);
      }
      else
      {
          /* Use Timer17 for LED2 and LED3 */
          TIM_StartWithDuration_Timer17(duration_ms, LED_Timer17Callback);
      }
  }
  
  /**
    * @brief  Initialize Timer16 and Timer17 for LED flashing
    * @retval None
    */
  static void LED_InitTimer(void)
  {
    /* Initialize Timer16 for LED1 flashing */
    htim16.Init.Period = 100 - 1;  /* Default 100us period */
    htim16.Init.Prescaler = (SystemCoreClock / 10000) - 1;  /* 10kHz */
    htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim16.Init.RepetitionCounter = 0;
    htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
    {
        Error_Handler();
    }
    
    /* Initialize Timer17 for LED2/3 flashing */
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
    * @brief  LED timer callback - called when LED flash duration expires (legacy)
    * @retval None
    */
  static void LED_TimerCallback(void)
  {
      /* Turn off all LEDs for robustness */
      LED_AllOff();
  }
  
  /**
    * @brief  LED Timer16 callback - called when LED1 flash duration expires
    * @retval None
    */
  static void LED_Timer16Callback(void)
  {
      /* Turn off LED1 specifically */
      LED_Off(&hled1);
  }
  
  /**
    * @brief  LED Timer17 callback - called when LED2/3 flash duration expires
    * @retval None
    */
  static void LED_Timer17Callback(void)
  {
      /* Turn off LED2 and LED3 */
      LED_Off(&hled2);
      LED_Off(&hled3);
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

