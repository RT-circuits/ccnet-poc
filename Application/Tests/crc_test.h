/**
  ******************************************************************************
  * @file           : crc_test.h
  * @brief          : CRC test module header file
  *                   Contains CRC test definitions and prototypes
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __CRC_TEST_H
#define __CRC_TEST_H

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
void CRC_Test_A_CalculateWithoutCRC(void);
void CRC_Test_B_CalculateWithCRC(void);
void CRC_Test_C_AppendCRC(void);
void CRC_Test_E_ValidateCRC(void);
void CRC_RunAllTests(void);

#ifdef __cplusplus
}
#endif

#endif /* __CRC_TEST_H */
