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
extern TIM_HandleTypeDef htim16;  /* Timer16 handle from main.c */
extern TIM_HandleTypeDef htim17;  /* Timer17 handle from main.c */

/* Timer callback system */
typedef void (*timer_callback_t)(void);
static timer_callback_t timer16_callback = NULL;
static timer_callback_t timer17_callback = NULL;

/* Private function prototypes -----------------------------------------------*/
static void TIM_StopTimer16(void);
static void TIM_StopTimer17(void);

/* Exported functions --------------------------------------------------------*/


/**
  * @brief  Start timer with duration and callback (uses timer17 by default)
  * @param  duration_ms: duration in milliseconds
  * @param  callback: function to call when timer expires
  * @retval None
  */
void TIM_StartWithDuration(uint16_t duration_ms, void (*callback)(void))
{
    TIM_StartWithDuration_Timer17(duration_ms, callback);
}

/**
  * @brief  Start timer16 with duration and callback
  * @param  duration_ms: duration in milliseconds
  * @param  callback: function to call when timer expires
  * @retval None
  */
void TIM_StartWithDuration_Timer16(uint16_t duration_ms, void (*callback)(void))
{
    /* Stop any existing timer first */
    HAL_TIM_Base_Stop_IT(&htim16);
    
    /* Clear any pending timer interrupts */
    __HAL_TIM_CLEAR_IT(&htim16, TIM_IT_UPDATE);
    
    /* Register the callback */
    timer16_callback = callback;
    
    /* Calculate the number of timer cycles needed for the duration */
    uint32_t timer_cycles = duration_ms * 10;  /* Convert ms to timer cycles (10kHz = 10 cycles per ms) */
    
    /* Set the timer autoreload value for the duration */
    __HAL_TIM_SET_AUTORELOAD(&htim16, timer_cycles - 1);
    
    /* Reset timer counter to zero */
    __HAL_TIM_SET_COUNTER(&htim16, 0);
    
    /* Start timer in interrupt mode */
    HAL_TIM_Base_Start_IT(&htim16);
}

/**
  * @brief  Start timer17 with duration and callback
  * @param  duration_ms: duration in milliseconds
  * @param  callback: function to call when timer expires
  * @retval None
  */
void TIM_StartWithDuration_Timer17(uint16_t duration_ms, void (*callback)(void))
{
    /* Stop any existing timer first */
    HAL_TIM_Base_Stop_IT(&htim17);
    
    /* Clear any pending timer interrupts */
    __HAL_TIM_CLEAR_IT(&htim17, TIM_IT_UPDATE);
    
    /* Register the callback */
    timer17_callback = callback;
    
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
  * @brief  Stop the timer (stops timer17 by default)
  * @retval None
  */
void TIM_StopTimer(void)
{
    TIM_StopTimer17();
}

/**
  * @brief  Stop timer16
  * @retval None
  */
static void TIM_StopTimer16(void)
{
    /* Stop the timer */
    HAL_TIM_Base_Stop_IT(&htim16);
    
    /* Clear any pending timer interrupts */
    __HAL_TIM_CLEAR_IT(&htim16, TIM_IT_UPDATE);
    
    /* Clear callback */
    timer16_callback = NULL;
}

/**
  * @brief  Stop timer17
  * @retval None
  */
static void TIM_StopTimer17(void)
{
    /* Stop the timer */
    HAL_TIM_Base_Stop_IT(&htim17);
    
    /* Clear any pending timer interrupts */
    __HAL_TIM_CLEAR_IT(&htim17, TIM_IT_UPDATE);
    
    /* Clear callback */
    timer17_callback = NULL;
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim: TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM16)
    {
        /* Stop the timer */
        HAL_TIM_Base_Stop_IT(&htim16);
        
        /* Reset timer counter to zero */
        __HAL_TIM_SET_COUNTER(&htim16, 0);
        
        /* Call the registered callback if it exists */
        if (timer16_callback != NULL)
        {
            timer16_callback();
        }
        
        /* Clear callback */
        timer16_callback = NULL;
    }
    else if (htim->Instance == TIM17)
    {
        /* Stop the timer */
        HAL_TIM_Base_Stop_IT(&htim17);
        
        /* Reset timer counter to zero */
        __HAL_TIM_SET_COUNTER(&htim17, 0);
        
        /* Call the registered callback if it exists */
        if (timer17_callback != NULL)
        {
            timer17_callback();
        }
        
        /* Clear callback */
        timer17_callback = NULL;
    }
}

/* Private functions ---------------------------------------------------------*/
