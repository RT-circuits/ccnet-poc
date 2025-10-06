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
#include "proto.h"
#include "uart.h"
#include "usb.h"
#include "log.h"
#include "config.h"
#include "btn.h"
#include "nvm.h"
#include "message.h"
#include "../Tests/tests.h"

#define LOG_LEVEL LOG_LEVEL_INFO

/* Message sending macros ----------------------------------------------------*/
#define REQUEST(opcode, data, length) APP_SendMessage(&if_downstream, opcode, data, length, 0)
#define REQUEST_DMA(opcode, data, length) APP_SendMessage(&if_downstream, opcode, data, length, 1)
#define RESPOND(opcode, data, length) APP_SendMessage(&if_upstream, opcode, data, length, 0)

/* Private variables ---------------------------------------------------------*/
static uint8_t dma_tx_buffer[256];

/* LED instances */
LED_HandleTypeDef hled1 = {LD1_GPIO_Port, LD1_Pin, LED_STATE_UNKNOWN};
LED_HandleTypeDef hled2 = {LD2_GPIO_Port, LD2_Pin, LED_STATE_UNKNOWN};
LED_HandleTypeDef hled3 = {LD3_GPIO_Port, LD3_Pin, LED_STATE_UNKNOWN};

/* Protocol state management */
typedef enum {
    POLL_IDLE = 0,
    POLL_SENDING,
    POLL_SENT
} poll_state_t;

typedef struct {
    poll_state_t poller_state;
    uint32_t last_poll_time;
} protocol_state_t;

static protocol_state_t ds = {POLL_IDLE, 0};
static protocol_state_t us = {POLL_IDLE, 0};

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
static message_parse_result_t APP_WaitForDownstreamMessage(uint32_t timeout_ms);
static void APP_ProcessDownstreamPolling(void);
static void APP_SendMessage(interface_config_t* interface, uint8_t opcode, uint8_t* data, uint8_t data_length, uint8_t use_dma);
static void my_memcpy(uint8_t* dest, const uint8_t* src, uint32_t length);
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
    LOG_SetLevel(LOG_LEVEL);

    /* Initialize messages for UART reception */
    /* CCNET: We receive TX commands from bill validator */
    /* ID003: We receive RX responses from bill validator */
    MESSAGE_Init(&upstream_msg, PROTO_CCNET, MSG_DIR_TX);
    MESSAGE_Init(&downstream_msg, PROTO_ID003, MSG_DIR_RX);
    
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
    uint8_t downstream_opcode;

    /* Process config/reset button */
    BTN_ProcessConfigResetButton();
    
    /* If config menu is active, don't process other functions */
    if (BTN_IsConfigMenuActive())
    {
        /* Process configuration menu */
        CONFIG_ProcessMenu();
        return; /* Exit early - don't process USB status messages */
    }
    
    /* Process downstream polling */
    APP_ProcessDownstreamPolling();
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
                
                switch (upstream_msg.opcode)
                {
                    case CCNET_POLL:
                        RESPOND(CCNET_STATUS_INITIALIZE, NULL, 0);
                        break;
                        
                    case CCNET_RESET:
                        REQUEST(ID003_RESET, NULL, 0);
                        if ((downstream_opcode = APP_WaitForDownstreamMessage(10)) != MSG_NO_MESSAGE)
                        {
                            RESPOND(CCNET_STATUS_ACK, NULL, 0);
                        }
                        else
                        {
                            RESPOND(CCNET_STATUS_NAK, NULL, 0);
                        }
                        break;
                        
                    default:
                        if (PROTO_SupportedCmd(upstream_msg.opcode))
                        {
                            LOG_Warn("Supported CCNET opcode received but not implemented");
                        }
                        else
                        {
                            LOG_Warn("Unsupported CCNET opcode received");
                        }
                        break;
                }
                break;
                
            case MSG_CRC_INVALID:
                // TODO: send NACK message
                /* 2.6.4 CCNET documenation*/
                LOG_Warn("Upstream IN message CRC invalid");
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

    /* Flush USB TX ring buffer */
    USB_Flush();
    
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

/**
  * @brief  Wait for downstream message with timeout
  * @param  timeout_ms: Timeout in milliseconds
  * @retval message_parse_result_t: MSG_NO_MESSAGE if timeout, or parse result
  */
static message_parse_result_t APP_WaitForDownstreamMessage(uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();
    
    while (1)
    {
        /* Check for downstream data and parse if available */
        if (UART_CheckForDownstreamData())
        {
            /* Parse the received message */
            MESSAGE_Parse(&downstream_msg);
            /* return opcode */
            return downstream_msg.opcode;
        }
        
        /* Check if timeout has occurred */
        if ((HAL_GetTick() - start_tick) >= timeout_ms)
        {
            /* Timeout occurred */
            return MSG_NO_MESSAGE;    /* 0x00 is not a valid id003 opcode */
        }
    }
}

/**
  * @brief  Send a message to specified interface
  * @param  interface: Pointer to interface configuration (upstream or downstream)
  * @param  opcode: Message opcode
  * @param  data: Pointer to message data (NULL if no data)
  * @param  data_length: Length of data (0 if no data)
  * @retval None
  */
static void APP_SendMessage(interface_config_t* interface, uint8_t opcode, uint8_t* data, uint8_t data_length, uint8_t use_dma)
{
    message_t tx_msg;
    message_direction_t direction;
    
    /* Determine direction based on interface */
    if (interface == &if_downstream) {
        direction = MSG_DIR_TX;  /* TX to downstream device */
    } else {
        direction = MSG_DIR_RX;  /* RX as seen by upstream controller */
    }
    
    /* Create message ready for transmission */
    tx_msg = MESSAGE_Create(interface->protocol, direction, opcode, data, data_length);

    /* Log the message */
    LOG_Debug("app.c: Sending message");
    LOG_Proto(&tx_msg);

    /* transmit message */
    if (!use_dma)
    {
        UART_TransmitMessage(interface, &tx_msg);
    }
    else
    {
        /* Transmit raw message data */
        my_memcpy(dma_tx_buffer, tx_msg.raw, tx_msg.length);
        HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(interface->phy.uart_handle, dma_tx_buffer, tx_msg.length);

        if (status != HAL_OK) {
            if (status == HAL_BUSY) LOG_Error("UART_TransmitMessage: Transmission failed - HAL BUSY");
            else LOG_Error("UART_TransmitMessage: Transmission failed");
        } else {
            LOG_Debug("app.c: uart tx using dma OK");
        }
    }
    

}

/**
  * @brief  Process downstream polling based on configured period
  * @retval None
  */
static void APP_ProcessDownstreamPolling(void)
{
    uint32_t current_time = HAL_GetTick();
    
    switch (ds.poller_state)
    {
        case POLL_IDLE:
            /* Check if polling period has expired */
            if ((current_time - ds.last_poll_time) >= if_downstream.datalink.polling_period_ms)
            {
                /* Set state to sending and send status request */
                ds.poller_state = POLL_SENDING;
                REQUEST_DMA(ID003_STATUS_REQ, NULL, 0);
                                
                /* Set state to sent */
                ds.poller_state = POLL_SENT;
                ds.last_poll_time = current_time;
            }
            break;
            
        case POLL_SENDING:
            /* Wait for DMA completion */
            ds.poller_state = POLL_SENT;
            break;
            
        case POLL_SENT:
            /* Reset to idle state */
            ds.poller_state = POLL_IDLE;
            break;
            
        default:
            /* Reset to idle state on unknown state */
            ds.poller_state = POLL_IDLE;
            break;
    }
}

/**
  * @brief  Simple memory copy function
  * @param  dest: Destination buffer
  * @param  src: Source buffer
  * @param  length: Number of bytes to copy
  * @retval None
  */
static void my_memcpy(uint8_t* dest, const uint8_t* src, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        dest[i] = src[i];
    }
}

