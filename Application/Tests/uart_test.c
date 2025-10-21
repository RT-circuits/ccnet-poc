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
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_uart.h"
#include "uart.h"
#include "log.h"
#include "message.h"
#include "usb.h"

/* Private variables ---------------------------------------------------------*/
static uint8_t uart2_sequence = 0; // 0=first, 1=second, 2=third (infinite)
static message_t test_receive_message;


/* Private functions --------------------------------------------------------*/
void UART_TEST_A_BasicTx2(void);
void UART_TEST_B_ListenUpstream(void);
void UART_TEST_C_BlockRx(void);
void UART_TEST_D_CCNET_MessageReception(void);

/**
  * @brief  Run all UART tests
  * @retval None
  */
void UART_RunAllTests(void)
{
  UART_TEST_A_BasicTx2();  
  /* Run comprehensive CCNET message reception test */
    // UART_TEST_D_CCNET_MessageReception();
    
    /* Run UART test B - listen on upstream interface */
    // UART_TEST_B_ListenUpstream();
    // UART_TEST_C_BlockRx();
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
  * @brief  UART DMA test sequence - sends data on 2 UARTs every 100ms
  * @retval None
  */
void UART_TEST_A_BasicTx2(void)
{
    while (1)
    {
        // UART1,2,3: Send 'UART_1' in ASCII
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*)"UART_1", 6);
        HAL_UART_Transmit_DMA(&huart2, (uint8_t*)"UART_2", 6);        
        HAL_UART_Transmit(&huart3, (uint8_t*)"UART_3", 6, 100);
        
        // Wait X ms before next transmission
        HAL_Delay(100);
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
    
    /* Initialize message for UART reception */
    /* CCNET: We receive TX commands from bill validator */
    MESSAGE_Init(&received_message, PROTO_CCNET, MSG_DIR_TX);
    
    /* Initialize UART for upstream interface using new implementation */
    UART_Init(&if_upstream, &received_message);
    
    /* Send test message to indicate test is running */
    LOG_Info("UART Test B: Listening on upstream interface (CCNET protocol)...");
    
    while (1)
    {
        /* Check for upstream messages */
        if (UART_CheckForUpstreamData())
        {
            /* Log the received message contents using LOG_Proto */
            LOG_Proto(&received_message);
        }
        
        /* Small delay to prevent busy waiting */
        HAL_Delay(1);
    }
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
  * @brief  CCNET message reception test
  * @retval None
  */
void UART_TEST_D_CCNET_MessageReception(void)
{
    LOG_SetLevel(LOG_LEVEL_INFO);
    LOG_Info("CCNET Message Reception Test - UART1");
    
    /* Initialize test message for UART reception */
    /* CCNET: We receive TX commands from bill validator */
    MESSAGE_Init(&test_receive_message, PROTO_CCNET, MSG_DIR_TX);
    
    /* Initialize UART for upstream interface */
    UART_Init(&if_upstream, &test_receive_message);
    
    while (1)
    {
        /* Check for upstream messages */
        if (UART_CheckForUpstreamData())
        {
            /* Parse the received message */
            message_parse_result_t parse_result = MESSAGE_Parse(&test_receive_message);
            
            
            if (parse_result == MSG_OK)
            {
                /* Log successful parsing */
                LOG_Proto(&test_receive_message);
            }
            else
            {
                /* Log parsing failure */
                LOG_Proto(&test_receive_message);
                LOG_Warn("Parse failed");
            }
        }
        
        /* Small delay to prevent busy waiting */
        HAL_Delay(1);
    }
}

void UART_TEST_E(void)
{
	/* structure: dest addr, data length, src addr, header (cmd), checksum
	 * dest 0: broadcase
	 * data length: 0 for simple poll. Total length = data length + 5
	 * src 1: default
	 * cmd: 254 for simple poll
	 * checksum: total sum mod 256 is 0
	 */
//	  uint8_t tx_buffer[6] = {0,0,1,254,0x29, 0x07};
	  uint8_t tx_buffer[] = {80,0,1,254,177};

  while(1){
	LOG_Debug("Sending simple poll");
	USB_Flush();
    HAL_UART_Transmit(&huart3, (uint8_t*) tx_buffer, sizeof(tx_buffer), 100);
    HAL_Delay(100);
  }
}
