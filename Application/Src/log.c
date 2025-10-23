/**
  ******************************************************************************
  * @file           : log.c
  * @brief          : Log module implementation
  *                   USB-based logging system
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "log.h"
#include "usb.h"
#include "proto.h"
#include <stdio.h>  /* For snprintf */


/* Private variables ---------------------------------------------------------*/
static log_level_t current_log_level = LOG_LEVEL_INFO;
static uint32_t log_counter = 0;
static uint8_t log_initialized = 0;
static uint32_t last_proto_log_time = 0;

/* Private function prototypes -----------------------------------------------*/
static void LOG_PrintHeader(log_level_t level);
static void LOG_PrintTimestamp(void);
static void LOG_PrintLevel(log_level_t level);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize log module
  * @retval None
  */
void LOG_Init(void)
{
    log_counter = 0;
    current_log_level = LOG_LEVEL_DEBUG;
    log_initialized = 1;
    
    // Wait a bit more to ensure USB is fully ready
    HAL_Delay(100);
    
}

/**
  * @brief  Set log level
  * @param  level: log level to set
  * @retval None
  */
void LOG_SetLevel(log_level_t level)
{
    current_log_level = level;
}

/**
  * @brief  Log info message - takes varargs
  * @param  message: message string
  * @retval None
  */
void LOG_Info(const char* message)
{
    if (current_log_level >= LOG_LEVEL_INFO)
    {
        LOG_PrintHeader(LOG_LEVEL_INFO);
        USB_TransmitString(message);
        USB_TransmitString("\r\n");
        log_counter++;
    }
}

/**
  * @brief  Log info message with uint32_t value without using snprintf as a test. I do not add vsnprintf
  * @param  message: Message string
  * @param  value: uint32_t value to append
  * @retval None
  */
void LOG_InfoUint(const char* message, uint32_t value)
{
    if (current_log_level >= LOG_LEVEL_INFO)
    {
        LOG_PrintHeader(LOG_LEVEL_INFO);
        USB_TransmitString(message);
        
        char value_str[16];
        utils_uint32_to_string(value, value_str, sizeof(value_str));
        USB_TransmitString(value_str);
        USB_TransmitString("\r\n");
        log_counter++;
    }
}

/**
  * @brief  Log error message
  * @param  message: message string
  * @retval None
  */
void LOG_Error(const char* message)
{
    if (!log_initialized) return;
    
    if (current_log_level >= LOG_LEVEL_ERROR)
    {
        LOG_PrintHeader(LOG_LEVEL_ERROR);
        USB_TransmitString(message);
        USB_TransmitString("\r\n");
        log_counter++;
    }
}

/**
  * @brief  Log warning message
  * @param  message: message string
  * @retval None
  */
void LOG_Warn(const char* message)
{
    if (current_log_level >= LOG_LEVEL_WARN)
    {
        LOG_PrintHeader(LOG_LEVEL_WARN);
        USB_TransmitString(message);
        USB_TransmitString("\r\n");
        log_counter++;
    }
}

/**
  * @brief  Log protocol message
  * @param  msg: pointer to message structure
  * @retval None
  */
void LOG_Proto(const message_t* msg)
{
    if (current_log_level >= LOG_LEVEL_PROTO)
    {
        uint32_t current_time = HAL_GetTick();
        
        /* Add blank line if more than 100ms since last protocol log */
        if ((last_proto_log_time > 0 && (current_time - last_proto_log_time) > 100) ||
            msg->opcode == ID003_CURRENCY_ASSIGN_REQ && msg->direction == MSG_DIR_TX)
        {
            USB_TransmitString("\r\n");
        }
        
        last_proto_log_time = current_time;
        
        // LOG_PrintHeader(LOG_LEVEL_PROTO);
        
        // Print protocol
        switch (msg->protocol)
        {
            case PROTO_CCNET:
                USB_TransmitString("UP   ");
                break;
            default:
                USB_TransmitString("DOWN ");
                break;
        }
        
        // Print direction (TX/RX)
        if (msg->direction == MSG_DIR_TX)
        {
            USB_TransmitString(">: ");
        }
        else
        {
            USB_TransmitString("<: ");
        }
        
        // Print opcode ASCII name with padding
        const char* opcode_ascii = MESSAGE_GetOpcodeASCII(msg);
        USB_TransmitString(opcode_ascii);
        
        // Pad to 40 characters for alignment using snprintf
        uint8_t name_len = 0;
        while (opcode_ascii[name_len] != '\0') name_len++;
        char pad_buffer[41];  // 40 spaces + null terminator
        snprintf(pad_buffer, sizeof(pad_buffer), "%*s", 40 - name_len, "");
        USB_TransmitString(pad_buffer);
        
        // Print raw bytes as hex using snprintf
        char hex_buffer[4];  // "XX " + null terminator
        for (uint8_t i = 0; i < msg->length; i++)
        {
            snprintf(hex_buffer, sizeof(hex_buffer), "%02X ", msg->raw[i]);
            USB_TransmitString(hex_buffer);
        }
        USB_TransmitString("\r\n");
        log_counter++;
    }
}


/**
  * @brief  Log debug message
  * @param  message: message string
  * @retval None
  */
void LOG_Debug(const char* message)
{
    if (current_log_level >= LOG_LEVEL_DEBUG)
    {
        LOG_PrintHeader(LOG_LEVEL_DEBUG);
        USB_TransmitString(message);
        USB_TransmitString("\r\n");
        log_counter++;
    }
}



/**
  * @brief  Log raw string (no formatting)
  * @param  str: string to log
  * @retval None
  */
void LOG_Raw(const char* str)
{
    USB_TransmitString(str);
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Print log header with timestamp and level
  * @param  level: log level
  * @retval None
  */
static void LOG_PrintHeader(log_level_t level)
{
    LOG_PrintTimestamp();
    LOG_PrintLevel(level);
}

/**
  * @brief  Print timestamp
  * @retval None
  */
static void LOG_PrintTimestamp(void)
{
    // No timestamp - removed numbers from logging
}

/**
  * @brief  Print log level
  * @param  level: log level
  * @retval None
  */
static void LOG_PrintLevel(log_level_t level)
{
    switch (level)
    {
        case LOG_LEVEL_ERROR:
            USB_TransmitString("ERROR: ");
            break;
        case LOG_LEVEL_WARN:
            USB_TransmitString("WARN:  ");
            break;
        case LOG_LEVEL_PROTO:
            USB_TransmitString("PROTO: ");
            break;
        case LOG_LEVEL_INFO:
            USB_TransmitString("INFO:  ");
            break;
        case LOG_LEVEL_DEBUG:
            USB_TransmitString("  DEBUG: ");
            break;
        case LOG_LEVEL_VERBOSE:
            USB_TransmitString("VERB:  ");
            break;
        default:
            USB_TransmitString("LOG:   ");
            break;
    }
}
