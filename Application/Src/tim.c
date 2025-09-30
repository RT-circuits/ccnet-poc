/**
  ******************************************************************************
  * @file           : tim.c
  * @brief          : Timer driver implementation
  *                   Timer control functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "tim.h"

/* Private variables ---------------------------------------------------------*/
extern TIM_HandleTypeDef htim17;  /* Timer17 handle from main.c */

/* Timer callback system */
typedef void (*timer_callback_t)(void);
static timer_callback_t timer_callback = NULL;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/


/**
  * @brief  Start timer with duration and callback
  * @param  duration_ms: duration in milliseconds
  * @param  callback: function to call when timer expires
  * @retval None
  */
void TIM_StartWithDuration(uint16_t duration_ms, void (*callback)(void))
{
    /* Stop any existing timer first */
    HAL_TIM_Base_Stop_IT(&htim17);
    
    /* Clear any pending timer interrupts */
    __HAL_TIM_CLEAR_IT(&htim17, TIM_IT_UPDATE);
    
    /* Small delay to ensure timer is fully stopped */
    HAL_Delay(1);
    
    /* Register the callback */
    timer_callback = callback;
    
    /* Calculate the number of timer cycles needed for the duration */
    uint32_t timer_cycles = duration_ms * 10;  /* Convert ms to timer cycles (10kHz = 10 cycles per ms) */
    
    /* Set the timer autoreload value for the duration */
    __HAL_TIM_SET_AUTORELOAD(&htim17, timer_cycles - 1);
    
    /* Reset timer counter to zero */
    __HAL_TIM_SET_COUNTER(&htim17, 0);
    
    /* Start timer in interrupt mode */
    HAL_TIM_Base_Start_IT(&htim17);
}

/**
  * @brief  Stop the timer
  * @retval None
  */
void TIM_StopTimer(void)
{
    /* Stop the timer */
    HAL_TIM_Base_Stop_IT(&htim17);
    
    /* Clear any pending timer interrupts */
    __HAL_TIM_CLEAR_IT(&htim17, TIM_IT_UPDATE);
    
    /* Clear callback */
    timer_callback = NULL;
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim: TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM17)
    {
        /* Stop the timer */
        HAL_TIM_Base_Stop_IT(&htim17);
        
        /* Reset timer counter to zero */
        __HAL_TIM_SET_COUNTER(&htim17, 0);
        
        /* Call the registered callback if it exists */
        if (timer_callback != NULL)
        {
            timer_callback();
        }
        
        /* Clear callback */
        timer_callback = NULL;
    }
}

/* Private functions ---------------------------------------------------------*/
