/**
  ******************************************************************************
  * @file           : uart.c
  * @brief          : UART driver implementation using datalink configuration
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "uart.h"
#include "log.h"
#include "app.h"
#include "message.h"
#include "stm32g4xx_hal_uart.h"

/* Private variables ---------------------------------------------------------*/

/**
  * @brief  UART receive state enumeration
  */
typedef enum {
    UART_STATE_WAIT_SYNC1,
    UART_STATE_WAIT_SYNC2,
    UART_STATE_WAIT_LENGTH,
    UART_STATE_WAIT_DATA
} UART_State_t;

/**
  * @brief  UART interface structure
  */
typedef struct {
    UART_HandleTypeDef *huart;
    uint8_t sync_length;              /* Number of sync bytes (1 or 2) */
    uint8_t length_offset;            /* Length offset (5 for cctalk (positive nr))*/
    uint8_t sync_bytes[2];         /* Sync bytes */
    UART_State_t state;
    uint8_t rx_buffer[256];        /* Receive buffer */
    uint16_t rx_index;
    uint8_t length;
    uint32_t last_tick;
    uint8_t rx_byte;
    uint8_t data_ready;            /* Flag for main loop to process received data */
    interface_config_t* interface; /* Reference to interface configuration */
    message_t* message;            /* Message structure to populate */
} UART_Interface_t;

/* Global instances for the two UART interfaces */
UART_Interface_t uart_intf1;
UART_Interface_t uart_intf2;

/* Exported variables --------------------------------------------------------*/
uint8_t downstream_rx_flag = 0;


/* Exported functions --------------------------------------------------------*/

/**
  * @brief  UART receive complete callback (called from HAL interrupt)
  * @param  huart: UART handle
  * @retval None
  */
void UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    datalink_config_t datalink;
    UART_Interface_t *intf = NULL;  /* used for context */
    if (huart == uart_intf1.huart) {
        intf = &uart_intf1; /* Upstream */
        datalink = if_upstream.datalink;
    } else if (huart == uart_intf2.huart) { /* Downstream */
        intf = &uart_intf2;
        datalink = if_downstream.datalink;
    }
    if (intf == NULL) return;
    /* set datalink parameters in context */
    intf->sync_length = datalink.sync_length;
    intf->sync_bytes[0] = datalink.sync_byte1;
    intf->sync_bytes[1] = datalink.sync_byte2;
    intf->length_offset = datalink.length_offset;

    uint32_t current_tick = HAL_GetTick();
    uint32_t timeout_ms = datalink.inter_byte_timeout_ms;
    
    /* Check for timeout */
    if (intf->state != UART_STATE_WAIT_SYNC1 && (current_tick - intf->last_tick > timeout_ms)) {
        intf->state = UART_STATE_WAIT_SYNC1;
        intf->rx_index = 0;
    }
    intf->last_tick = current_tick;

    uint8_t byte = intf->rx_byte;

    switch (intf->state) {
    case UART_STATE_WAIT_SYNC1:
        if (byte == intf->sync_bytes[0]) {
            intf->rx_buffer[0] = byte;
            intf->rx_index = 1;
            if (intf->sync_length == 1) {
                intf->state = UART_STATE_WAIT_LENGTH;
            } else {
                intf->state = UART_STATE_WAIT_SYNC2;
            }
        }
        /* Else discard and stay in state */
        break;

    case UART_STATE_WAIT_SYNC2:
        if (byte == intf->sync_bytes[1]) {
            intf->rx_buffer[1] = byte;
            intf->rx_index = 2;
            intf->state = UART_STATE_WAIT_LENGTH;
        } else {
            /* Handle potential sync overlap/start */
            if (byte == intf->sync_bytes[0]) {
                intf->rx_buffer[0] = byte;
                intf->rx_index = 1;
                intf->state = UART_STATE_WAIT_SYNC2;
            } else {
                intf->state = UART_STATE_WAIT_SYNC1;
                intf->rx_index = 0;
            }
        }
        break;

    case UART_STATE_WAIT_LENGTH:
        intf->length = byte + intf->interface->datalink.length_offset;
        intf->rx_buffer[intf->rx_index++] = byte;
        
        /* Validate length */
        if (intf->length < (intf->sync_length + 1)) {
            /* Invalid length (too small) */
            intf->state = UART_STATE_WAIT_SYNC1;
            intf->rx_index = 0;
        } else {
            intf->state = UART_STATE_WAIT_DATA;
            /* Check if message is already complete (no data bytes) */
            if (intf->rx_index == intf->length) {
                /* Copy received data to message structure */
                if (intf->message != NULL) {
                    /* Clear the entire message buffer first */
                    for (int i = 0; i < 256; i++) {
                        intf->message->raw[i] = 0;
                    }
                    /* Copy only the received bytes */
                    intf->message->length = intf->length;
                    for (uint16_t i = 0; i < intf->length; i++) {
                        intf->message->raw[i] = intf->rx_buffer[i];
                    }
                }
                intf->data_ready = 1;
                intf->state = UART_STATE_WAIT_SYNC1;
                intf->rx_index = 0;
            }
        }
        break;

    case UART_STATE_WAIT_DATA:
        intf->rx_buffer[intf->rx_index++] = byte;
        if (intf->rx_index == intf->length) {
            /* Copy received data to message structure */
            if (intf->message != NULL) {
                /* Clear the entire message buffer first */
                for (int i = 0; i < 256; i++) {
                    intf->message->raw[i] = 0;
                }
                /* Copy only the received bytes */
                intf->message->length = intf->length;
                for (uint16_t i = 0; i < intf->length; i++) {
                    intf->message->raw[i] = intf->rx_buffer[i];
                }
            }
            intf->data_ready = 1;
            intf->state = UART_STATE_WAIT_SYNC1;
            intf->rx_index = 0;
        }
        break;
    }

    /* Restart reception for next byte */
    HAL_UART_Receive_IT(huart, &intf->rx_byte, 1);
}

/**
  * @brief  Check for upstream received data
  * @retval uint8_t: 1 if data ready, 0 if no data
  */
uint8_t UART_CheckForUpstreamData(void)
{
    /* Process upstream messages */
    if (uart_intf1.data_ready) {
        uart_intf1.data_ready = 0;
        return 1; /* Data ready */
    }
    return 0; /* No data */
}

/**
  * @brief  Check for downstream received data
  * @retval uint8_t: 1 if data ready, 0 if no data
  */
uint8_t UART_CheckForDownstreamData(void)
{
    /* Process downstream messages */
    if (uart_intf2.data_ready) {
        uart_intf2.data_ready = 0;
        LOG_Info("UART2 data received");
        return 1; /* Data ready */
    }
    return 0; /* No data */
}

/**
  * @brief  Initialize UART interface using datalink configuration
  * @param  interface: Interface configuration
  * @param  message: Message structure to populate with received data
  * @retval None
  */
void UART_Init(interface_config_t* interface, message_t* message)
{
    UART_Interface_t *intf = NULL;
    
    /* Determine which interface to initialize */
    if (interface->phy.uart_handle == &huart1) {
        intf = &uart_intf1;
    } else if (interface->phy.uart_handle == &huart2) {
        intf = &uart_intf2;
    }
    
    if (intf == NULL) return;
    
    /* Initialize interface structure */
    intf->huart = interface->phy.uart_handle;
    intf->sync_length = interface->datalink.sync_length;
    intf->sync_bytes[0] = interface->datalink.sync_byte1;
    intf->sync_bytes[1] = interface->datalink.sync_byte2;
    intf->state = UART_STATE_WAIT_SYNC1;
    intf->rx_index = 0;
    intf->data_ready = 0;
    intf->last_tick = HAL_GetTick();
    intf->interface = interface;
    intf->message = message;
    
    /* Clear receive buffer with zeros */
    for (int i = 0; i < 256; i++) {
        intf->rx_buffer[i] = 0;
    }
    
    /* Start receiving first byte */
    HAL_UART_Receive_IT(intf->huart, &intf->rx_byte, 1);
}

/**
  * @brief  Transmit message via UART
  * @param  interface: Interface configuration
  * @param  message: Message structure containing raw data to transmit
  * @retval None
  */
void UART_TransmitMessage(interface_config_t* interface, message_t* message)
{
    if (interface == NULL || message == NULL) {
        LOG_Error("UART_TransmitMessage: Invalid parameters");
        return;
    }
    
    if (interface->phy.uart_handle == NULL) {
        LOG_Error("UART_TransmitMessage: Invalid UART handle");
        return;
    }
    
    if (message->length == 0) {
        LOG_Warn("UART_TransmitMessage: Message length is zero");
        return;
    }

    /* Transmit raw message data */
    HAL_StatusTypeDef status = HAL_UART_Transmit(interface->phy.uart_handle, message->raw, message->length, 100);

    if (status != HAL_OK) {
    	if (status == HAL_BUSY) LOG_Error("UART_TransmitMessage: Transmission failed - HAL BUSY");
    	else LOG_Error("UART_TransmitMessage: Transmission failed");
    } else {
        LOG_Info("UART_TransmitMessage: Message transmitted successfully");
    }
}

/* Private functions ---------------------------------------------------------*/
