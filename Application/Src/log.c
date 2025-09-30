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

/* Private variables ---------------------------------------------------------*/
static log_level_t current_log_level = LOG_LEVEL_INFO;
static uint32_t log_counter = 0;
static uint8_t log_initialized = 0;

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
    current_log_level = LOG_LEVEL_INFO;
    log_initialized = 1;
    
    // Wait a bit more to ensure USB is fully ready
    HAL_Delay(100);
    
    // Send startup message - use simple approach
    USB_TransmitString("\r\n=== LOG MODULE INITIALIZED ===\r\n");
    USB_TransmitString("Log level: INFO\r\n");
    USB_TransmitString("Output: USB VCP\r\n");
    USB_TransmitString("================================\r\n\r\n");
    
    // Test log immediately
    LOG_Info("Log module test message");
}

/**
  * @brief  Set log level
  * @param  level: log level to set
  * @retval None
  */
void LOG_SetLevel(log_level_t level)
{
    current_log_level = level;
    LOG_Info("Log level changed to %d", level);
}

/**
  * @brief  Log error message
  * @param  format: format string
  * @param  ...: variable arguments
  * @retval None
  */
void LOG_Error(const char* format, ...)
{
    if (!log_initialized) return;
    
    if (current_log_level >= LOG_LEVEL_ERROR)
    {
        LOG_PrintHeader(LOG_LEVEL_ERROR);
        // For now, just send the format string (no printf support)
        USB_TransmitString(format);
        USB_TransmitString("\r\n");
        log_counter++;
    }
}

/**
  * @brief  Log warning message
  * @param  format: format string
  * @param  ...: variable arguments
  * @retval None
  */
void LOG_Warn(const char* format, ...)
{
    if (current_log_level >= LOG_LEVEL_WARN)
    {
        LOG_PrintHeader(LOG_LEVEL_WARN);
        USB_TransmitString(format);
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
        LOG_PrintHeader(LOG_LEVEL_PROTO);
        
        // Print protocol info
        switch (msg->protocol)
        {
            case PROTO_ID003:
                USB_TransmitString("ID003 ");
                break;
            case PROTO_CCTALK:
                USB_TransmitString("CCTALK ");
                break;
            case PROTO_CCNET:
                USB_TransmitString("CCNET ");
                break;
            default:
                USB_TransmitString("UNKNOWN ");
                break;
        }
                
        // Print all bytes of raw message data as hex (no truncation)
        char hex_chars[] = "0123456789ABCDEF";
        for (uint8_t i = 0; i < msg->length; i++)
        {
            uint8_t high_nibble = (msg->raw[i] >> 4) & 0x0F;
            uint8_t low_nibble = msg->raw[i] & 0x0F;
            char hex_buffer[3] = {hex_chars[high_nibble], hex_chars[low_nibble], 0};
            USB_TransmitString(hex_buffer);
            USB_TransmitString(" ");
        }
        
        USB_TransmitString("\r\n");
        log_counter++;
    }
}

/**
  * @brief  Log info message
  * @param  format: format string
  * @param  ...: variable arguments
  * @retval None
  */
void LOG_Info(const char* format, ...)
{
    if (!log_initialized) return;
    
    if (current_log_level >= LOG_LEVEL_INFO)
    {
        LOG_PrintHeader(LOG_LEVEL_INFO);
        USB_TransmitString(format);
        USB_TransmitString("\r\n");
        log_counter++;
    }
}

/**
  * @brief  Log debug message
  * @param  format: format string
  * @param  ...: variable arguments
  * @retval None
  */
void LOG_Debug(const char* format, ...)
{
    if (current_log_level >= LOG_LEVEL_DEBUG)
    {
        LOG_PrintHeader(LOG_LEVEL_DEBUG);
        USB_TransmitString(format);
        USB_TransmitString("\r\n");
        log_counter++;
    }
}

/**
  * @brief  Log verbose message
  * @param  format: format string
  * @param  ...: variable arguments
  * @retval None
  */
void LOG_Verbose(const char* format, ...)
{
    if (current_log_level >= LOG_LEVEL_VERBOSE)
    {
        LOG_PrintHeader(LOG_LEVEL_VERBOSE);
        USB_TransmitString(format);
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
    uint32_t tick = HAL_GetTick();
    
    // Simple timestamp format: [TICK]
    USB_TransmitString("[");
    
    // Simple tick display
    if (tick < 10) USB_TransmitString("00");
    else if (tick < 100) USB_TransmitString("0");
    else if (tick < 1000) USB_TransmitString("1");
    else if (tick < 2000) USB_TransmitString("2");
    else if (tick < 3000) USB_TransmitString("3");
    else if (tick < 4000) USB_TransmitString("4");
    else if (tick < 5000) USB_TransmitString("5");
    else USB_TransmitString(">5");
    
    USB_TransmitString("] ");
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
            USB_TransmitString("DEBUG: ");
            break;
        case LOG_LEVEL_VERBOSE:
            USB_TransmitString("VERB:  ");
            break;
        default:
            USB_TransmitString("LOG:   ");
            break;
    }
}
