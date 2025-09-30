/**
  ******************************************************************************
  * @file           : usb.c
  * @brief          : USB Virtual COM Port implementation
  *                   USB CDC communication functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usb.h"

/* USB constants */
#define USBD_OK 0

/* Forward declaration for CDC functions */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);


/* Simple strlen implementation */
static uint16_t my_strlen(const char* str)
{
    uint16_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

/* Private variables ---------------------------------------------------------*/
static uint32_t usb_tx_counter = 0;
static uint32_t usb_rx_counter = 0;
static uint8_t usb_connected = 0;
static uint8_t usb_tx_busy = 0;

/* Input line buffer for configuration menu only */
#define USB_INPUT_BUFFER_SIZE 32
static char usb_input_buffer[USB_INPUT_BUFFER_SIZE];
static uint8_t usb_input_pos = 0;
static uint8_t usb_input_ready = 0;

/* USB status message */
static uint32_t last_usb_status_message_time = 0;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize USB VCP
  * @retval None
  */
void USB_Init(void)
{
    // USB device is already initialized in main.c via MX_USB_Device_Init()

    // Just reset our counters
    usb_tx_counter = 0;
    usb_rx_counter = 0;
    usb_connected = 1; // Assume connected after init
}

/**
  * @brief  Process USB VCP communication
  * @retval None
  */
void USB_Process(void)
{
    // Check if USB is connected and configured
    // For now, assume connected if USB device exists
    usb_connected = 1; // Will work when USB is properly connected
}

/**
  * @brief  Transmit string via USB VCP
  * @param  str: pointer to string
  * @retval None
  */
void USB_TransmitString(const char* str)
{
    uint16_t length = my_strlen(str);
    CDC_Transmit_FS((uint8_t*)str, length);
    usb_tx_counter++;
    
    // No blocking delay - let USB handle transmission asynchronously
    HAL_Delay(3);
}

/**
  * @brief  Transmit data via USB VCP
  * @param  data: pointer to data buffer
  * @param  length: data length
  * @retval None
  */
void USB_TransmitBytes(uint8_t* data, uint16_t length)
{
    CDC_Transmit_FS(data, length);
    usb_tx_counter++;
    
    // No blocking delay - let USB handle transmission asynchronously
}

/**
  * @brief  Check if USB VCP is connected
  * @retval 1 if connected, 0 if not connected
  */
uint8_t USB_IsConnected(void)
{
    return usb_connected;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  USB CDC receive handler - called by USB middleware when data is received
  * @param  Buf: pointer to received data buffer
  * @param  Len: length of received data
  * @retval None
  */
void USB_CDC_RxHandler(uint8_t* Buf, uint32_t Len)
{
    if (Buf == NULL || Len == 0)
    {
        return;
    }
    
    // Process each received character for configuration menu input only
    for (uint32_t i = 0; i < Len; i++)
    {
        uint8_t ch = Buf[i];
        
        // Handle special characters (no echo in interrupt context)
        if (ch == '\r' || ch == '\n') {
            // Enter pressed - process the input line
            if (usb_input_pos > 0) {
                usb_input_buffer[usb_input_pos] = '\0'; // Null terminate
                usb_input_ready = 1;
            }
        } else if (ch == '\b' || ch == 127) {
            // Backspace - remove last character
            if (usb_input_pos > 0) {
                usb_input_pos--;
            }
        } else if (ch >= 32 && ch <= 126) {
            // Printable character - add to input buffer
            if (usb_input_pos < USB_INPUT_BUFFER_SIZE - 1) {
                usb_input_buffer[usb_input_pos++] = ch;
            }
        }
    }
}


/**
  * @brief  Check if input line is ready
  * @retval 1 if input is ready, 0 if not
  */
uint8_t USB_IsInputReady(void)
{
    return usb_input_ready;
}


/**
  * @brief  Get input line
  * @param  buffer: buffer to store input line
  * @param  max_length: maximum buffer length
  * @retval Number of characters in input line
  */
uint8_t USB_GetInputLine(char* buffer, uint8_t max_length)
{
    if (usb_input_ready && buffer != NULL)
    {
        uint8_t copy_length = (usb_input_pos < max_length - 1) ? usb_input_pos : max_length - 1;
        
        // Copy input line to user buffer
        for (uint8_t i = 0; i < copy_length; i++)
        {
            buffer[i] = usb_input_buffer[i];
        }
        buffer[copy_length] = '\0';
        
        // Clear the input ready flag and reset position
        usb_input_ready = 0;
        usb_input_pos = 0;
        
        return copy_length;
    }
    
    return 0;
}

/**
  * @brief  Process USB status message (send every 2 seconds)
  * @retval None
  */
void USB_ProcessStatusMessage(void)
{
    /* Send USB running message every 2 seconds */
    uint32_t current_time = HAL_GetTick();
    if (current_time - last_usb_status_message_time >= 2000)
    {
        USB_TransmitString("USB running - Time: ");
        
        // Simple integer to string conversion
        uint32_t seconds = current_time / 1000;
        char time_str[16];
        uint8_t pos = 0;
        
        if (seconds == 0) {
            time_str[pos++] = '0';
        } else {
            uint32_t temp = seconds;
            uint8_t digits = 0;
            while (temp > 0) {
                temp /= 10;
                digits++;
            }
            temp = seconds;
            for (uint8_t i = 0; i < digits; i++) {
                time_str[digits - 1 - i] = '0' + (temp % 10);
                temp /= 10;
            }
            pos = digits;
        }
        time_str[pos] = '\0';
        
        USB_TransmitString(time_str);
        USB_TransmitString("s\r\n");
        last_usb_status_message_time = current_time;
    }
}

