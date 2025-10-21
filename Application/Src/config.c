/**
  ******************************************************************************
  * @file           : config.c
  * @brief          : Configuration module implementation
  *                   Configuration management and CLI interface
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "config-ui.h"
#include "usb.h"
#include "log.h"
#include "led.h"
#include "nvm.h"
#include "utils.h"  /* For utils_memcpy */
#include <stdio.h>  /* For snprintf */

/* External LED handle */
extern LED_HandleTypeDef hled3;


/* Private variables ---------------------------------------------------------*/

/* Global configuration settings */
config_settings_t g_config;



/* Private function prototypes -----------------------------------------------*/
static void CONFIG_SetPhy(interface_config_t* interface);
static void CONFIG_SetDataLink(interface_config_t* interface);
static nvm_result_t CONFIG_SerializeToBuffer(uint8_t* buffer, uint32_t* buffer_size);
static nvm_result_t CONFIG_DeserializeFromBuffer(const uint8_t* buffer, uint32_t buffer_size);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize configuration module
  * @retval None
  */
void CONFIG_Init(void)
{
    /* Point to default UART interface objects set in app.c as fallbacks */
    /* These provide default values if reading from flash fails */
    g_config.upstream = &if_upstream;
    g_config.downstream = &if_downstream;
    
    g_config.usb_logging_enabled = 1;
    g_config.log_level = LOG_LEVEL_INFO;
    
    /* Initialize bill table to all zeros */
    for (int i = 0; i < 8; i++)
    {
        g_config.bill_table[i] = 0;
    }
    
    /* Load settings from NVM and store in if_upstream and if_downstream */
    CONFIG_LoadFromNVM();
    CONFIG_SetPhy(&if_upstream);
    CONFIG_SetPhy(&if_downstream);
    CONFIG_SetDataLink(&if_upstream);
    CONFIG_SetDataLink(&if_downstream);
    
}

/**
  * @brief  Load configuration from non-volatile memory
  * @retval None
  */
void CONFIG_LoadFromNVM(void)
{
    nvm_result_t result;
    uint8_t buffer[512];
    uint32_t buffer_size;
    
    /* Load serialized data from NVM */
    result = NVM_ReadConfigData(buffer, sizeof(buffer), &buffer_size);
    if (result == NVM_OK)
    {
        /* Deserialize configuration from buffer */
        result = CONFIG_DeserializeFromBuffer(buffer, buffer_size);
        if (result != NVM_OK)
        {
            LOG_Warn("Failed to deserialize configuration data, using defaults");
        }
        else {
            /* Apply log level before logging success message */
            LOG_SetLevel(g_config.log_level);
            LOG_Info("Configuration loaded from flash successfully");
        }
    }
    else
    {
        LOG_Warn("Failed to load configuration from NVM, using defaults");
    }
}

/**
  * @brief  Set protocol-specific phy (uart) configuration for interface
  * @param  interface: Pointer to interface configuration structure
  * @note   Sets sync bytes, length offset, CRC length, and timeout based on protocol.
  *         Polling period is preserved from flash storage.
  * @retval None
  */
static void CONFIG_SetPhy(interface_config_t* interface)
{
    switch (interface->protocol)
    {
        case PROTO_CCNET:
            interface->phy.uart_handle = &huart1;
            break;
        case PROTO_ID003:
            interface->phy.uart_handle = &huart2;
            break;
        case PROTO_CCTALK:
            interface->phy.uart_handle = &huart3;
            break;
        default:
            LOG_Error("Unknown protocol");
            break;
    }
}

/**
  * @brief  Set protocol-specific datalink configuration for interface
  * @param  interface: Pointer to interface configuration structure
  * @note   Sets sync bytes, length offset, CRC length, and timeout based on protocol.
  *         Polling period is preserved from flash storage.
  * @retval None
  */
  static void CONFIG_SetDataLink(interface_config_t* interface)
  {
      /* Set data link configuration in interface object based on protocol read from flash */
      /* only polling period (relevant for downstream) is stored in flash */
      switch (interface->protocol)
      {
          case PROTO_CCNET:
              interface->datalink.sync_length = 2;
              interface->datalink.sync_byte1 = 0x02;
              interface->datalink.sync_byte2 = 0x03;
              interface->datalink.length_offset = 0;
              interface->datalink.crc_length = 2;
              interface->datalink.inter_byte_timeout_ms = 5;
              break;
          case PROTO_ID003:
              interface->datalink.sync_length = 1;
              interface->datalink.sync_byte1 = 0xFC;
              interface->datalink.sync_byte2 = 0x00;
              interface->datalink.length_offset = 0;
              interface->datalink.crc_length = 2;
              interface->datalink.inter_byte_timeout_ms = 5;
              break;
          case PROTO_CCTALK:
              interface->datalink.sync_length = 1;
              interface->datalink.sync_byte1 = 0x50;
              interface->datalink.sync_byte2 = 0x00;
              interface->datalink.length_offset = 5;
              interface->datalink.crc_length = 1;
              interface->datalink.inter_byte_timeout_ms = 5;
              interface->datalink.cctalk_echo_byte_count = 0;
              break;
          default:
              LOG_Error("Unknown protocol");
              break;
      }
  }

/**
  * @brief  Save configuration to non-volatile memory
  * @retval None
  */
void CONFIG_SaveToNVM(void)
{
    nvm_result_t result;
    uint8_t buffer[512];
    uint32_t buffer_size;
    
    /* Serialize configuration to buffer */
    result = CONFIG_SerializeToBuffer(buffer, &buffer_size);
    if (result == NVM_OK)
    {
        /* Save serialized data to NVM */
        result = NVM_WriteConfigData(buffer, buffer_size);
        if (result == NVM_OK)
        {
            USB_TransmitString("Configuration saved successfully!\r\n");
        }
        else
        {
            LOG_Error("Failed to save configuration to NVM");
            USB_TransmitString("Error: Failed to save configuration!\r\n");
        }
    }
    else
    {
        LOG_Error("Failed to serialize configuration");
        USB_TransmitString("Error: Failed to serialize configuration!\r\n");
    }
}


/**
  * @brief  Serialize configuration to buffer
  * @param  buffer: Pointer to buffer to store serialized data
  * @param  buffer_size: Pointer to receive actual buffer size used
  * @retval NVM operation result
  */
static nvm_result_t CONFIG_SerializeToBuffer(uint8_t* buffer, uint32_t* buffer_size)
{
    if (buffer == NULL || buffer_size == NULL)
    {
        return NVM_INVALID_PARAM;
    }
    
    uint32_t offset = 0;
    
    /* Serialize upstream interface configuration */
    if (g_config.upstream != NULL)
    {
        utils_memcpy(&buffer[offset], (const uint8_t*)g_config.upstream, sizeof(interface_config_t));
        offset += sizeof(interface_config_t);
    }
    else
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Serialize downstream interface configuration */
    if (g_config.downstream != NULL)
    {
        utils_memcpy(&buffer[offset], (const uint8_t*)g_config.downstream, sizeof(interface_config_t));
        offset += sizeof(interface_config_t);
    }
    else
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Serialize other configuration fields */
    buffer[offset++] = g_config.usb_logging_enabled;
    buffer[offset++] = g_config.log_level;
    
    /* Serialize bill table */
    for (int i = 0; i < 8; i++)
    {
        buffer[offset++] = g_config.bill_table[i];
    }
    
    *buffer_size = offset;
    return NVM_OK;
}

/**
  * @brief  Deserialize configuration from buffer
  * @param  buffer: Pointer to buffer containing serialized data
  * @param  buffer_size: Size of buffer data
  * @retval NVM operation result
  */
static nvm_result_t CONFIG_DeserializeFromBuffer(const uint8_t* buffer, uint32_t buffer_size)
{
    if (buffer == NULL)
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Calculate expected buffer size: 2x interface_config_t + 2 bytes + 8 bytes bill table */
    uint32_t expected_size = (2 * sizeof(interface_config_t)) + 2 + 8;
    
    if (buffer_size != expected_size)
    {
        LOG_Error("Buffer size does not match expected config size");
        return NVM_INVALID_PARAM;
    }
    
    uint32_t offset = 0;
    
    /* Deserialize upstream interface configuration */
    if (g_config.upstream != NULL)
    {
        utils_memcpy((uint8_t*)g_config.upstream, &buffer[offset], sizeof(interface_config_t));
        offset += sizeof(interface_config_t);
    }
    else
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Deserialize downstream interface configuration */
    if (g_config.downstream != NULL)
    {
        utils_memcpy((uint8_t*)g_config.downstream, &buffer[offset], sizeof(interface_config_t));
        offset += sizeof(interface_config_t);
    }
    else
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Deserialize other configuration fields */
    g_config.usb_logging_enabled = buffer[offset++];
    g_config.log_level = buffer[offset++];
    
    /* Deserialize bill table */
    for (int i = 0; i < 8; i++)
    {
        g_config.bill_table[i] = buffer[offset++];
    }
    
    return NVM_OK;
}

