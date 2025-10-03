/**
  ******************************************************************************
  * @file           : app.c
  * @brief          : Application layer implementation
  *                   Main application logic and state management
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "app.h"
#include "led.h"
#include "uart.h"
#include "usb.h"
#include "log.h"
#include "config.h"
#include "btn.h"
#include "nvm.h"
#include "message.h"
#include "../Tests/tests.h"

/* Private variables ---------------------------------------------------------*/

/* LED instances */
LED_HandleTypeDef hled1 = {LD1_GPIO_Port, LD1_Pin, LED_STATE_UNKNOWN};
LED_HandleTypeDef hled2 = {LD2_GPIO_Port, LD2_Pin, LED_STATE_UNKNOWN};
LED_HandleTypeDef hled3 = {LD3_GPIO_Port, LD3_Pin, LED_STATE_UNKNOWN};

/* Message structures for UART data reception */
message_t upstream_msg;    /* CCNET messages from upstream */
message_t downstream_msg;  /* ID003 messages from downstream */




/* Default interface objects might both flash banks be corrupted */
/* CCNET interface on UART1 - Upstream communication */
interface_config_t if_upstream = {
    .protocol = PROTO_CCNET,           /* CCNET protocol */
    .role = ROLE_BILL_VALIDATOR,        /* Acting as bill validator */
    .phy = {
        .baudrate = 9600,               /* 9600 baud communication */
        .parity = UART_PARITY_NONE,     /* No parity */
        .uart_polarity = POLARITY_NORMAL, /* Normal signal polarity */
        .uart_handle = &huart1          /* UART1 hardware handle */
    },
};

/* ID003 interface on UART2 - Downstream communication */
interface_config_t if_downstream = {
    .protocol = PROTO_ID003,            /* ID003 protocol */
    .role = ROLE_CONTROLLER,            /* Acting as controller */
    .phy = {
        .baudrate = 9600,               /* 9600 baud communication */
        .parity = UART_PARITY_EVEN,     /* Even parity */
        .uart_polarity = POLARITY_NORMAL, /* Normal signal polarity */
        .uart_handle = &huart2          /* UART2 hardware handle */
    },
    .datalink.polling_period_ms = 100,       /* 100ms polling period */
};

/* Private function prototypes -----------------------------------------------*/
message_parse_result_t APP_CheckForUpstreamMessage(void);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  MCU reset function
  * @note   This function is called from HAL_GPIO_EXTI_Callback in main.c
  * @retval None
  */
void APP_MCUReset(void)
{
    // Perform software reset
    HAL_NVIC_SystemReset();
}



/**
  * @brief  Initialize application
  * @retval None
  */
void APP_Init(void)
{
    /* Initialize LEDs first (blocking) */
    LED_Init();

    
    /* Initialize USB VCP */
    USB_Init();
    
    /* Wait for USB to be ready - longer delay for consistency */
    HAL_Delay(500);
    
    
    /* Initialize Log module */
    LOG_Init();

    /* Initialize messages for UART reception */
    /* CCNET: We receive TX commands from bill validator */
    /* ID003: We receive RX responses from bill validator */
    MESSAGE_Init(&upstream_msg, PROTO_CCNET, MSG_DIR_TX, 0);
    MESSAGE_Init(&downstream_msg, PROTO_ID003, MSG_DIR_RX, 0);
    
    /* Run all enabled tests */
    TESTS_RunAll();

    /* Initialize NVM module - DISABLED FOR TESTING */
    NVM_Init();
    
    /* Initialize UARTs with message structures */
    UART_Init(&if_upstream, &upstream_msg);
    UART_Init(&if_downstream, &downstream_msg);
       
    
    /* Initialize Configuration module */
    CONFIG_Init();
    
    /* Initialize Button module */
    BTN_Init();
    
    /* Log application startup */
    LOG_Info("Application started");

    

    
}

/**
  * @brief  Main application process
  * @retval None
  */
void APP_Process(void)
{
    message_parse_result_t msg_received_status;

    /* Process config/reset button */
    BTN_ProcessConfigResetButton();
    
    /* If config menu is active, don't process other functions */
    if (BTN_IsConfigMenuActive())
    {
        /* Process configuration menu */
        CONFIG_ProcessMenu();
        return; /* Exit early - don't process USB status messages */
    }
    /* Check for upstream message */
    if ((msg_received_status = APP_CheckForUpstreamMessage()) != MSG_NO_MESSAGE)
    {
        /* message received */
        switch (msg_received_status)
        {
            case MSG_OK:
                // TODO: process upstream message
                LOG_Info("CCNET message received OK");
                LOG_Proto(&upstream_msg);
                break;
                
            case MSG_CRC_INVALID:
                // TODO: send NACK message
                /* 2.6.4 CCNET documenation*/
                LOG_Warn("Upstream message CRC invalid");
                break;
                
            case MSG_UNKNOWN_OPCODE:
            case MSG_DATA_MISSING_FOR_OPCODE:
                // TODO: send 0x30 message
                /* 2.3.5 CCNET documenation*/
                LOG_Warn("Upstream message unknown opcode or data missing for opcode");
                break;
                
            default:
                // No action needed
                LOG_Warn("Upstream message parse failed without CRC invalid or unknown opcode or data missing for opcode");
                break;
        }
    }


    
    /* Process USB VCP communication */
    USB_Process();
    
    USB_ProcessStatusMessage();
}


/* Private functions ---------------------------------------------------------*/


/**
  * @brief  Check for upstream message and parse if available
  * @retval message_parse_result_t: MSG_NO_MESSAGE if no data, or parse result
  */
message_parse_result_t APP_CheckForUpstreamMessage(void)
{
    /* Check for upstream data and parse if available */
    if (UART_CheckForUpstreamData())
    {
        /* Parse the received message */
        return MESSAGE_Parse(&upstream_msg);
    }
    
    /* No message received */
    return MSG_NO_MESSAGE;
}

