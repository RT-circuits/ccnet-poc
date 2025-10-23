/**
  ******************************************************************************
  * @file           : tests.c
  * @brief          : Test suite runner
  *                   Centralized test execution and management
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "tests.h"
#include "crc_test.h"
#include "usb_test.h"
#include "uart_test.h"
#include "msg_test.h"

/* Private variables ---------------------------------------------------------*/

/* Test configuration - easily enable/disable test suites */
#define ENABLE_CRC_TESTS           0
#define ENABLE_PROTO_CONVERTER_TESTS 0
#define ENABLE_USB_TESTS           0
#define ENABLE_UART_TESTS          0
#define ENABLE_CCTALK_TESTS        0
#define ENABLE_MESSAGE_TESTS       0

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Run all enabled test suites
  * @retval None
  */
void TESTS_RunAll(void)
{
#if ENABLE_CRC_TESTS
    /* Run CRC calculation and validation tests */
    CRC_RunAllTests();
#endif

#if ENABLE_PROTO_CONVERTER_TESTS
    /* Run protocol converter tests */
    PROTO_CONVERTER_Test_CompleteFlow();
#endif

#if ENABLE_USB_TESTS
    /* Run USB tests */
    USB_RunAllTests();
#endif

#if ENABLE_UART_TESTS
    /* Run UART test sequence */
    UART_RunAllTests();
#endif

#if ENABLE_CCTALK_TESTS
    /* Run UART test sequence */
    UART_TEST_E();
#endif

#if ENABLE_MESSAGE_TESTS
    /* Run message creation test */
    MSG_TEST_CreateCCTalkMessage();
#endif
}

/* Private functions ---------------------------------------------------------*/
