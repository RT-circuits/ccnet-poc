/**
  ******************************************************************************
  * @file           : usb_test.c
  * @brief          : USB test module implementation
  *                   Simple USB test functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usb_test.h"
#include "usb.h"
#include "log.h"

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Test: Send 1000-byte readable string in indefinite loop
  * @retval None
  */
void USB_Test_1000ByteString(void)
{
    // Create a 1000-byte string with repeating "0123456789" pattern
    char test_string[1000]; // Exactly 1000 bytes, no null terminator
    int pos = 0;
    
    // Fill with repeating "0123456789" pattern
    const char* pattern = "0123456789";
    int pattern_len = 10;
    
    // Fill 990 bytes with repeating pattern (99 repetitions)
    for (int rep = 0; rep < 99; rep++) {
        for (int i = 0; i < pattern_len; i++) {
            test_string[pos++] = pattern[i];
        }
    }
    
    // Add the final 10 characters: "012345678!" (exclamation at byte 1000)
    const char* ending = "012345678!";
    for (int i = 0; i < 10; i++) {
        test_string[pos++] = ending[i];
    }
    
    // Indefinite loop with 10-second periods
    while (1) {
        // Single USB_TransmitBytes call with exactly 1000 bytes
        USB_TransmitBytes((uint8_t*)test_string, 1000);
        
        // Wait 10 seconds
        HAL_Delay(10000);
    }
}

/**
  * @brief  Test USB FIFO ring buffer with incremental messages
  * @retval None
  */
void USB_Test_FifoRingBuffer(void)
{
	HAL_Delay(100);
    LOG_Info("USB FIFO Ring Buffer Test");
    USB_Flush();
    
    /* Send 100 test messages */
    for (uint8_t i = 48; i < 128; i++) /* ASCII starting with '0' */
    {
        USB_Tx((uint8_t*)"test message", sizeof("test message")-1);
        USB_Tx((uint8_t*)&i, 1);
        USB_Tx((uint8_t*)"\r\n", 2);
        USB_Flush();
    }
    HAL_Delay(100);
    LOG_Info("USB FIFO test completed");
}

/**
  * @brief  Run all USB tests
  * @retval None
  */
void USB_RunAllTests(void)
{
//    USB_Test_1000ByteString();
    USB_Test_FifoRingBuffer();
}
