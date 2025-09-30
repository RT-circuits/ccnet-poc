/**
  ******************************************************************************
  * @file           : message_test.h
  * @brief          : Message test module header file
  *                   Message construction test functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __MESSAGE_TEST_H
#define __MESSAGE_TEST_H

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
void MESSAGE_Test_A_BasicConstruction(void);
void MESSAGE_Test_B_DataPayload(void);
void MESSAGE_Test_C_ProtocolSpecific(void);
void MESSAGE_RunAllTests(void);

#ifdef __cplusplus
}
#endif

#endif /* __MESSAGE_TEST_H */
