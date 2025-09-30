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
#include "../Tests/tests.h"

/* Private variables ---------------------------------------------------------*/

/* LED instances */
LED_HandleTypeDef hled1 = {LD1_GPIO_Port, LD1_Pin, LED_STATE_UNKNOWN};
LED_HandleTypeDef hled2 = {LD2_GPIO_Port, LD2_Pin, LED_STATE_UNKNOWN};
LED_HandleTypeDef hled3 = {LD3_GPIO_Port, LD3_Pin, LED_STATE_UNKNOWN};




/* Interface objects */
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
    .datalink = {
        .polling_period_ms = 100,       /* 100ms polling period */
        .sync_length = 2,               /* 2 sync bytes for CCNET */
        .sync_byte1 = 0x02,             /* First sync byte */
        .sync_byte2 = 0x03,             /* Second sync byte */
        .length_offset = 0,             /* No offset for CCNET */
        .crc_length = 2,                /* 2 CRC bytes */
        .inter_byte_timeout_ms = 5      /* 5ms inter-byte timeout */
    }
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
    .datalink = {
        .polling_period_ms = 100,       /* 100ms polling period */
        .sync_length = 1,               /* 1 sync byte for ID003 */
        .sync_byte1 = 0xFC,             /* Sync byte */
        .sync_byte2 = 0x00,             /* Not used for ID003 */
        .length_offset = 0,             /* No offset for ID003 */
        .crc_length = 2,                /* 2 CRC bytes */
        .inter_byte_timeout_ms = 5      /* 5ms inter-byte timeout */
    }
};

/* Private function prototypes -----------------------------------------------*/

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
    
    /* Send startup banner */
    USB_TransmitString("\r\n*****************\r\nUSB VCP initialized\r\n*****************\r\n\r\n");
    
    /* Test USB with simple message */
    USB_TransmitString("USB Test: Direct message\r\n");
    
    /* Initialize Log module */
    LOG_Init();

    /* Initialize UARTs */
    UART_Init(&if_upstream, NULL);
    UART_Init(&if_downstream, NULL);
    
    /* Run all enabled tests */
    TESTS_RunAll();

    /* Initialize NVM module - DISABLED FOR TESTING */
    NVM_Init();
    
    /* Initialize Configuration module */
    CONFIG_Init();
    
    /* Initialize Button module */
    BTN_Init();
    
    /* Log application startup */
    LOG_Info("Application started");

    

    
    /* Send startup message */
    CONFIG_BufferWrite("\r\n=== SYSTEM READY ===\r\n");
    CONFIG_BufferWrite("USB running - will echo input and send status every 2s\r\n");
    CONFIG_FlushBuffer();
}

/**
  * @brief  Main application process
  * @retval None
  */
void APP_Process(void)
{
    /* Process config/reset button */
    BTN_ProcessConfigResetButton();
    
    /* If config menu is active, don't process other functions */
    if (BTN_IsConfigMenuActive())
    {
        /* Process configuration menu */
        CONFIG_ProcessMenu();
        return; /* Exit early - don't process USB status messages */
    }
    
    /* Process USB VCP communication */
    USB_Process();
    
    USB_ProcessStatusMessage();
}


/* Private functions ---------------------------------------------------------*/

