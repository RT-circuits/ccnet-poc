/**
  ******************************************************************************
  * @file           : tim.h
  * @brief          : Timer driver header file
  *                   Contains timer control definitions and prototypes
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __TIM_H
#define __TIM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void TIM_Start(void);
void TIM_Stop(void);
void TIM_StartWithDuration(uint16_t duration_ms, void (*callback)(void));
void TIM_StartWithDuration_Timer16(uint16_t duration_ms, void (*callback)(void));
void TIM_StartWithDuration_Timer17(uint16_t duration_ms, void (*callback)(void));
void TIM_StopTimer(void);

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H */
