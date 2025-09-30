/**
  ******************************************************************************
  * @file           : usb_test.h
  * @brief          : USB test module header file
  *                   Simple USB test functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __USB_TEST_H
#define __USB_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported functions prototypes ---------------------------------------------*/
void USB_Test_1000ByteString(void);
void USB_RunAllTests(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_TEST_H */
