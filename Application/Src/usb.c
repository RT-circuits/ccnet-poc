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
#include "usbd_cdc_if.h"
#include <stdio.h>  /* For snprintf */
#include "utils.h"  /* For utils_memcpy */

/* USB constants */
#define USBD_OK 0
#define USB_TX_RINGBUFFER_SIZE 2048

/* Private variables ---------------------------------------------------------*/

/* USB TX Ring Buffer */
static uint8_t usb_tx_buffer[USB_TX_RINGBUFFER_SIZE];
static uint16_t usb_tx_head = 0;
static uint16_t usb_tx_tail = 0;
static uint8_t hostReadyFlag = 1; /* Start as ready */

/* Input line buffer for configuration menu only */
#define USB_INPUT_BUFFER_SIZE 32
static char usb_input_buffer[USB_INPUT_BUFFER_SIZE];
static uint8_t usb_input_pos = 0;
static uint8_t usb_input_ready = 0;

/* USB status message */
static uint32_t last_usb_status_message_time = 0;

/* Private function prototypes -----------------------------------------------*/
static uint16_t my_strlen(const char* str);

/* Callback functions --------------------------------------------------------*/

/**
  * @brief  CDC Transmission Complete Callbac - called by USB middleware when data is received (usbd_cdc_if.c)
  * @retval None
  */
void   USB_CDCTransmitCpltHandler(void)
{
    // Set ready flag to allow next transmission
    hostReadyFlag = 1;
}

/**
  * @brief  USB CDC receive handler - called by USB middleware when data is received (usbd_cdc_if.c)
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

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize USB VCP
  * @retval None
  */
void USB_Init(void)
{
    // USB device is already initialized in main.c via MX_USB_Device_Init()

    // Initialize ring buffer
    usb_tx_head = 0;
    usb_tx_tail = 0;
    hostReadyFlag = 1; /* Start as ready */

}



/**
  * @brief  Transmit string via USB VCP
  * @param  str: pointer to string
  * @retval None
  */
void USB_TransmitString(const char* str)
{
    if (str == NULL) return;
    uint16_t length = my_strlen(str);
    USB_Tx((uint8_t*)str, length);
}

/**
  * @brief  Transmit data via USB VCP
  * @param  data: pointer to data buffer
  * @param  length: data length
  * @retval None
  */
void USB_TransmitBytes(uint8_t* data, uint16_t length)
{
    if (data == NULL || length == 0) return;
    USB_Tx(data, length);
}



/* Private functions ---------------------------------------------------------*/


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
        utils_memcpy((uint8_t*)buffer, (uint8_t*)usb_input_buffer, copy_length);
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
        uint32_t seconds = current_time / 1000;
        char status_msg[32];
        
        snprintf(status_msg, sizeof(status_msg), "USB running - Time: %lus\r\n", (unsigned long)seconds);
        USB_TransmitString(status_msg);
        
        last_usb_status_message_time = current_time;
    }
}

/**
  * @brief  Add data to USB TX ring buffer
  * @param  buffer: pointer to data buffer
  * @param  length: length of data to add
  * @retval None
  */
void USB_Tx(uint8_t* buffer, uint16_t length)
{
    if (buffer == NULL || length == 0) return;
    
    for (uint16_t i = 0; i < length; i++)
    {
        uint16_t next_head = (usb_tx_head + 1) % USB_TX_RINGBUFFER_SIZE;
        
        /* Check if buffer is full */
        if (next_head == usb_tx_tail) break; /* Buffer full, drop data */
        
        usb_tx_buffer[usb_tx_head] = buffer[i];
        usb_tx_head = next_head;
    }
}

/**
  * @brief  Flush USB TX ring buffer if host is ready
  * @retval None
  */
void USB_Flush(void)
{
    if (!hostReadyFlag || usb_tx_head == usb_tx_tail) return; /* Host not ready or no data */
    
    /* Calculate how much data to send */
    uint16_t data_length;
    if (usb_tx_head > usb_tx_tail)
    {
        /* Linear case: data from tail to head */
        data_length = usb_tx_head - usb_tx_tail;
        
        /* Send data */
        CDC_Transmit_FS(&usb_tx_buffer[usb_tx_tail], data_length);
        usb_tx_tail += data_length;
        hostReadyFlag = 0; /* Will be set to 1 by USB_CDCTransmitCpltHandler callback */
    }
    else
    {
        /* Wrap-around case: data from tail to end of buffer */
        data_length = USB_TX_RINGBUFFER_SIZE - usb_tx_tail;
        
        /* Send data */
        CDC_Transmit_FS(&usb_tx_buffer[usb_tx_tail], data_length);
        usb_tx_tail = (usb_tx_tail + data_length) % USB_TX_RINGBUFFER_SIZE;
        hostReadyFlag = 0; /* Will be set to 1 by USB_CDCTransmitCpltHandler callback */
    }
}

/* Simple strlen implementation */
static uint16_t my_strlen(const char* str)
{
    uint16_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}
