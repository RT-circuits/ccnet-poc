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
#include "utils.h"
#include "../Tests/tests.h"

#define LOG_LEVEL LOG_LEVEL_INFO

/* Message sending macros ----------------------------------------------------*/
#define REQUEST(opcode, data, length) APP_SendMessage(&if_downstream, opcode, data, length, 0)
#define REQUEST_DMA(opcode, data, length) APP_SendMessage(&if_downstream, opcode, data, length, 1)
#define RESPOND(opcode, data, length) APP_SendMessage(&if_upstream, opcode, data, length, 0)
#define CREATE_RESP(msg) MESSAGE_Create(PROTO_CCNET, MSG_DIR_TX, msg.opcode, msg.data, msg.length);

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
    poll_state_t state;
    uint32_t last_poll_time;
    message_t last_msg;
    uint8_t last_opcode;
} poller_t;

typedef enum {
    DS_NO_RESPONSE = 0,
    DS_OK,
} downstream_state_t;

typedef struct {
    poller_t poller;
    downstream_state_t state;
    message_t last_req_msg; /* last request sent downstream to check for ID003 echo*/
} downstream_context_t;

downstream_context_t ds_context = {
    .poller.state = POLL_IDLE,
    .poller.last_msg = {0, 0, 0, NULL, 0},
    .poller.last_opcode = 0,
    .state = DS_NO_RESPONSE
};

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
message_parse_result_t APP_CheckForDownstreamMessage(void);
static message_parse_result_t APP_WaitForDownstreamMessage(uint32_t timeout_ms);
static void APP_DownstreamPolling(void);
static void APP_SendMessage(interface_config_t* interface, uint8_t opcode, uint8_t* data, uint8_t data_length, uint8_t use_dma);
static void APP_GetBillTable(void);
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
    APP_DownstreamPolling();
    
    /* Check for downstream message */
    if ((msg_received_status = APP_CheckForDownstreamMessage()) != MSG_NO_MESSAGE)
    {
        /* Update state on first message */
        if (ds_context.state == DS_NO_RESPONSE)
        {
            LOG_Debug("Downstream first message received");
            ds_context.state = DS_OK;
        }
        
        /* message received */
        switch (msg_received_status)
        {
            case MSG_OK:
                // TODO: process downstream message
                LOG_Debug("Downstream message received OK");
                LOG_Proto(&downstream_msg);
                if (PROTO_IsId003StatusCode(downstream_msg.opcode))
                {
                    LOG_Debug("Downstream ID003 status code parsed to upstream msg object");
                
                    
                }
                else
                {
                    LOG_Warn("Downstream message is not a ID003 status code");
                }

                  
                
                break;
                
            case MSG_CRC_INVALID:
                // TODO: send NACK message
                LOG_Warn("Downstream IN message CRC invalid");
                break;
                
            case MSG_UNKNOWN_OPCODE:
            break;
            case MSG_DATA_MISSING_FOR_OPCODE:
                // TODO: send error message
                LOG_Warn("Downstream message unknown opcode or data missing for opcode");
                break;
                
            default:
                // No action needed
                LOG_Warn("Downstream message parse failed without CRC invalid or unknown opcode or data missing for opcode");
                break;
        }
    }
    
    /* Check for upstream message */
    if ((msg_received_status = APP_CheckForUpstreamMessage()) != MSG_NO_MESSAGE)
    {
        message_t new_us_msg;      /* new upstream message created by mapping status code and data*/

        /* message received */
        switch (msg_received_status)
        {
            case MSG_OK:
                // TODO: process upstream message
                LOG_Debug("CCNET message received OK");
                LOG_Proto(&upstream_msg);
                
                switch (upstream_msg.opcode)
                {
                    case CCNET_ACK:                    /* 0x00 - ACK */
                        LOG_Debug("CCNET_ACK received");
                        break;

                    case CCNET_RESET:                  /* 0x30 - Reset */
                        REQUEST(ID003_RESET, NULL, 0);
                        if ((downstream_opcode = APP_WaitForDownstreamMessage(10)) != MSG_NO_MESSAGE)
                            RESPOND(CCNET_STATUS_ACK, NULL, 0);
                        else
                            RESPOND(CCNET_STATUS_NAK, NULL, 0);
                        break;

                    case CCNET_STATUS_REQUEST:         /* 0x31 - Get Status */
                        LOG_Warn("CCNET_STATUS_REQUEST not implemented");
                        break;

                    case CCNET_POLL:                   /* 0x33 - Poll */
                        /* check if downstream message has been received or polling is disabled */
                        if (ds_context.state != DS_NO_RESPONSE || if_downstream.datalink.polling_period_ms == 0)
                        {
                            /* If polling is disabled, manually request status */
                            if (if_downstream.datalink.polling_period_ms == 0)
                            {
                                REQUEST(ID003_STATUS_REQ, NULL, 0);
                                APP_WaitForDownstreamMessage(10);
                            }
                            
                            PROTO_MapStatusCode(&downstream_msg, &new_us_msg);   /* updates opcode and data */
                            CREATE_RESP(new_us_msg);
                            RESPOND(new_us_msg.opcode, new_us_msg.data, new_us_msg.length);
                        }
                        else
                        {
                            LOG_Warn("No bill validator connected");
                        }
                        break;

                    case CCNET_ENABLE_BILL_TYPES:      /* 0x34 - Enable Bill Types */
                        {
                            uint8_t enable_data[1] ={0};  /* 0: enable all bill types*/
                            REQUEST(ID003_INHIBIT, enable_data, 1);
                            APP_WaitForDownstreamMessage(10); /* wait for enable response. do nothing now*/

                            uint8_t inhibit_data[1] ={0};  /* 0: de-inhibit*/
                            REQUEST(ID003_INHIBIT, inhibit_data, 1);
                            APP_WaitForDownstreamMessage(10); /* wait for inhibit response to respond upstream*/
                            RESPOND(CCNET_STATUS_ACK, NULL, 0); /* todo to respond NAK */
                        }
                        break;

                    case CCNET_STACK:                  /* 0x35 - Stack */
                        LOG_Warn("CCNET_STACK not implemented");
                        break;

                    case CCNET_RETURN:                 /* 0x36 - Return */
                        LOG_Warn("CCNET_RETURN not implemented");
                        break;

                    case CCNET_IDENTIFICATION:         /* 0x37 - Identification */
                        LOG_Warn("CCNET_IDENTIFICATION not implemented");
                        break;

                    case CCNET_BILL_TABLE:             /* 0x41 - Get Bill Table */
                        APP_GetBillTable();
                        break;

                    case CCNET_NAK:                    /* 0xFF - NAK */
                        LOG_Warn("CCNET_NAK received");
                        break;

                    default:
                        if (IsSupportedCcnetCommand(upstream_msg.opcode))
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
    
    // USB_ProcessStatusMessage();
}

/**
  * @brief  Check for downstream message and parse if available
  * @retval message_parse_result_t: MSG_NO_MESSAGE if no data, or parse result
  */
  message_parse_result_t APP_CheckForDownstreamMessage(void)
  {
      /* Check for downstream data and parse if available */
      if (UART_CheckForDownstreamData())
      {
          /* Parse the received message */
          return MESSAGE_Parse(&downstream_msg);
      }
      
      /* No message received */
      return MSG_NO_MESSAGE;
  }

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
    
    LOG_Debug("In APP_WaitForDownstreamMessage");

    while (1)
    {
        /* Check for downstream data and parse if available */
        if (UART_CheckForDownstreamData())
        {
            /* Parse the received message */
            MESSAGE_Parse(&downstream_msg);
            LOG_Proto(&downstream_msg); 
            
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
        LED_Flash(&hled2, 10);
    } else {
        direction = MSG_DIR_RX;  /* RX as seen by upstream controller */
        LED_Flash(&hled1, 10);
    }
    
    /* Create message ready for transmission */
    tx_msg = MESSAGE_Create(interface->protocol, direction, opcode, data, data_length);
    if (interface == &if_downstream) {
        /* creaate a copy of last request message and store to check for echo*/
        message_t last_ds_msg;
        last_ds_msg = MESSAGE_Create(interface->protocol, MSG_DIR_RX, tx_msg.opcode, tx_msg.data, tx_msg.length);
        ds_context.last_req_msg = last_ds_msg;
    }

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
        utils_memcpy(dma_tx_buffer, tx_msg.raw, tx_msg.length);
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
static void APP_DownstreamPolling(void)
{
    uint32_t current_time = HAL_GetTick();
    
    /* Skip polling if disabled (period = 0) */
    if (if_downstream.datalink.polling_period_ms == 0)
    {
        return;
    }
    
    if ((current_time - ds_context.poller.last_poll_time) >= if_downstream.datalink.polling_period_ms)
        {
            /* Send status request */
            REQUEST_DMA(ID003_STATUS_REQ, NULL, 0);
                            
            /* Set state to sent */
            ds_context.poller.state = POLL_SENT;
            ds_context.poller.last_poll_time = current_time;
        }
}

/**
  * @brief  Get bill table from downstream validator
  * @retval None
  */
static void APP_GetBillTable(void)
{
    /* Check protocol type */
    if (if_downstream.protocol == PROTO_ID003)
    {
        /* Request currency assignment/bill table from ID003 validator */
        REQUEST(ID003_CURRENCY_ASSIGN_REQ, NULL, 0);
        
        /* Wait 10ms for response */
        if ((APP_WaitForDownstreamMessage(10)) == MSG_OK)
        {
            LOG_Debug("AGG_GET_BILL_TABLE: ok");
            LOG_Proto(&downstream_msg);
        }
        else {
            LOG_Warn("AGG_GET_BILL_TABLE: failed");
        }

    }
}


