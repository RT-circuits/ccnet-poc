/**
  ******************************************************************************
  * @file           : tests.h
  * @brief          : Test suite runner header file
  *                   Centralized test execution and management
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __TESTS_H
#define __TESTS_H

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
void TESTS_RunAll(void);

#ifdef __cplusplus
}
#endif

#endif /* __TESTS_H */
