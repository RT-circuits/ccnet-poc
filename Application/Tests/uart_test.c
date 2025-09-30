/**
  ******************************************************************************
  * @file           : uart_test.c
  * @brief          : Simple UART test sequence
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "uart_test.h"
#include "app.h"
#include "stm32g4xx_hal_uart.h"
#include "uart.h"
#include "log.h"

/* Private variables ---------------------------------------------------------*/
static uint8_t uart2_sequence = 0; // 0=first, 1=second, 2=third (infinite)
static message_t test_receive_message;


/* Exported functions --------------------------------------------------------*/


void UART_TEST_C_BlockRx(void);

/**
  * @brief  Run all UART tests
  * @retval None
  */
void UART_RunAllTests(void)
{
    /* Run UART test B - listen on upstream interface */
    UART_TEST_B_ListenUpstream();
    // UART_TEST_C_BlockRx();
}

void UART_TEST_C_BlockRx(void)
{
uint8_t rx_buffer[10];
extern UART_HandleTypeDef huart1;

  HAL_UART_Receive(&huart1, rx_buffer, 5, 5000);

  LOG_Info("Received uart1 data");
  while (1){
    HAL_UART_Receive(&huart1, rx_buffer, 5, 5000);
    LOG_Info("Received uart1 data");
    HAL_Delay(100);
  }
}

/**
  * @brief  UART test sequence - sends data on all 3 UARTs every 100ms
  * @retval None
  */
void UART_TEST_A_BasicTx(void)
{
    while (1)
    {
        // UART1,2,3: Send 'UART_1' in ASCII
        HAL_UART_Transmit(&huart1, (uint8_t*)"UART_1", 6, 100);
        HAL_UART_Transmit(&huart2, (uint8_t*)"UART_2", 6, 100);        
        HAL_UART_Transmit(&huart3, (uint8_t*)"UART_3", 6, 100);
        
        // Wait X ms before next transmission
        HAL_Delay(10);
    }
}

/**
  * @brief  UART test B - listens on upstream interface and logs messages
  * @retval None
  */
void UART_TEST_B_ListenUpstream(void)
{
    /* Set log level to INFO to ensure messages are displayed */
    LOG_SetLevel(LOG_LEVEL_INFO);
    LOG_Info("UART Test B: Starting UART message reception test");
    
    /* Create message structure for received data */
    message_t received_message;
    
    /* Initialize UART for upstream interface using new implementation */
    UART_Init(&if_upstream, &received_message);
    
    /* Send test message to indicate test is running */
    LOG_Info("UART Test B: Listening on upstream interface (CCNET protocol)...");
    
    while (1)
    {
        /* Process any received messages */
        UART_ProcessReceivedBytes();
        
        /* Check for completed messages */
        if (upstream_rx_flag)
        {
            upstream_rx_flag = 0;
            
            /* Log the received message contents using LOG_Proto */
            LOG_Proto(&received_message);
        }
        
        /* Small delay to prevent busy waiting */
        HAL_Delay(1);
    }
}
