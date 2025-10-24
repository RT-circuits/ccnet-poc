/**
  ******************************************************************************
  * @file           : msg_test.c
  * @brief          : Message test module implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "msg_test.h"
#include "message.h"
#include "uart.h"
#include "log.h"
#include "usb.h"
/* Exported functions --------------------------------------------------------*/

void MSG_TEST_CreateCCTalkMessage(void)

{
  LOG_Info("Creating CCTalk message and transmitting it");
  USB_Flush();
    message_t msg;
    
    // Create message with opcode 254, no data

    while (1){  
      msg = MESSAGE_Create(PROTO_CCTALK, MSG_DIR_TX, 254, NULL, 0);
      LOG_Proto(&msg);

        // Send over UART3
        HAL_UART_Transmit(&huart3, msg.raw, msg.length, 100);
        
        // Wait 100ms
        HAL_Delay(100);
        USB_Flush();
    }
}
