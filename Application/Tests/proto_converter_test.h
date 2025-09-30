/**
  ******************************************************************************
  * @file           : proto_converter_test.h
  * @brief          : Protocol converter test module header file
  *                   Protocol conversion flow test functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __PROTO_CONVERTER_TEST_H
#define __PROTO_CONVERTER_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/* Test state enumeration */
typedef enum {
    TEST_STATE_IDLE,
    TEST_STATE_RECEIVED_CCNET,
    TEST_STATE_SENT_ID003,
    TEST_STATE_RECEIVED_ID003_RESPONSE,
    TEST_STATE_SENT_CCNET_RESPONSE
} test_state_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
/* Global variables for debugger inspection */
extern volatile test_state_t test_current_state;
extern volatile uint8_t test_ccnet_received[6];
extern volatile uint8_t test_ccnet_received_length;
extern volatile uint8_t test_id003_to_send[256];
extern volatile uint8_t test_id003_to_send_length;
extern volatile uint8_t test_expected_id003_response;
extern volatile uint8_t test_received_id003_response;
extern volatile uint8_t test_ccnet_response_to_send[256];
extern volatile uint8_t test_ccnet_response_to_send_length;


/* Exported functions prototypes ---------------------------------------------*/
void PROTO_CONVERTER_Test_CompleteFlow(void);
void PROTO_CONVERTER_Test_ReceiveCCNETMessage(void);
void PROTO_CONVERTER_Test_SendID003Message(void);
void PROTO_CONVERTER_Test_ReceiveID003Response(void);
void PROTO_CONVERTER_Test_SendCCNETResponse(void);

#ifdef __cplusplus
}
#endif

#endif /* __PROTO_CONVERTER_TEST_H */
