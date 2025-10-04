/**
  ******************************************************************************
  * @file           : proto_converter_test.c
  * @brief          : Protocol converter test module implementation
  *                   Complete protocol conversion flow test
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "proto_converter_test.h"
#include "../Inc/message.h"
#include "../Inc/proto.h"

/* Private variables ---------------------------------------------------------*/

/* Test message: CCNET 02 03 06 33 DA 81 */
static const uint8_t test_ccnet_message[] = {0x02, 0x03, 0x06, 0x33, 0xDA, 0x81};

/* Global variables for debugger inspection */
volatile test_state_t test_current_state = TEST_STATE_IDLE;
volatile uint8_t test_ccnet_received[6] = {0};
volatile uint8_t test_ccnet_received_length = 0;
volatile uint8_t test_id003_to_send[256] = {0};
volatile uint8_t test_id003_to_send_length = 0;
volatile uint8_t test_expected_id003_response = 0;
volatile uint8_t test_received_id003_response = 0;
volatile uint8_t test_ccnet_response_to_send[256] = {0};
volatile uint8_t test_ccnet_response_to_send_length = 0;


/* Message objects for the test */
static message_t received_ccnet_msg;
static message_t id003_request_msg;
static message_t received_id003_response_msg;
static message_t ccnet_response_msg;

/* Private function prototypes -----------------------------------------------*/
static void PROTO_CONVERTER_ParseCCNETMessage(const uint8_t* raw_data, uint8_t length, message_t* msg);
static uint8_t PROTO_CONVERTER_GetCCNETOpcode(const uint8_t* raw_data, uint8_t length);
static uint8_t PROTO_CONVERTER_MapCCNETToID003(uint8_t ccnet_opcode);
static uint8_t PROTO_CONVERTER_MapID003ToCCNET(uint8_t id003_status);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Complete protocol conversion flow test
  * @retval None
  */
void PROTO_CONVERTER_Test_CompleteFlow(void)
{
    /* Step 1: Receive CCNET message */
    PROTO_CONVERTER_Test_ReceiveCCNETMessage();
    
    /* Step 2: Convert and send ID003 message */
    PROTO_CONVERTER_Test_SendID003Message();
    
    /* Step 3: Simulate receiving ID003 response */
    PROTO_CONVERTER_Test_ReceiveID003Response();
    
    /* Step 4: Convert and send CCNET response */
    PROTO_CONVERTER_Test_SendCCNETResponse();
}

/**
  * @brief  Test receiving CCNET message
  * @retval None
  */
void PROTO_CONVERTER_Test_ReceiveCCNETMessage(void)
{
    /* Copy received CCNET message */
    for (uint8_t i = 0; i < 6; i++)
    {
        test_ccnet_received[i] = test_ccnet_message[i];
    }
    test_ccnet_received_length = 6;
    
    /* Parse CCNET message */
    PROTO_CONVERTER_ParseCCNETMessage(test_ccnet_message, 6, &received_ccnet_msg);
    
    /* Update state */
    test_current_state = TEST_STATE_RECEIVED_CCNET;
    
    /* Breakpoint here to inspect received CCNET message */
    __NOP();
}

/**
  * @brief  Test converting and sending ID003 message
  * @retval None
  */
void PROTO_CONVERTER_Test_SendID003Message(void)
{
    /* Map CCNET opcode to ID003 opcode */
    uint8_t ccnet_opcode = PROTO_CONVERTER_GetCCNETOpcode(test_ccnet_received, test_ccnet_received_length);
    uint8_t id003_opcode = PROTO_CONVERTER_MapCCNETToID003(ccnet_opcode);
    
    /* Create ID003 message */
    id003_request_msg = MESSAGE_Create(PROTO_ID003, MSG_DIR_TX, id003_opcode, NULL, 0);
    
    /* Copy to debugger variables */
    for (uint8_t i = 0; i < id003_request_msg.length; i++)
    {
        test_id003_to_send[i] = id003_request_msg.raw[i];
    }
    test_id003_to_send_length = id003_request_msg.length;
    
    /* Set expected response based on the ID003 opcode */
    test_expected_id003_response = PROTO_CONVERTER_MapCCNETToID003(ccnet_opcode);
    if (id003_opcode == ID003_STATUS_REQ)
    {
        test_expected_id003_response = ID003_STATUS_IDLING; /* Most common response */
    }
    
    /* Update state */
    test_current_state = TEST_STATE_SENT_ID003;
    
    /* Breakpoint here to inspect ID003 message to send */
    __NOP();
}

/**
  * @brief  Test receiving ID003 response
  * @retval None
  */
void PROTO_CONVERTER_Test_ReceiveID003Response(void)
{
    /* Simulate receiving ID003 response (IDLING status) */
    uint8_t id003_response_raw[] = {0xFC, 0x05, 0x11, 0x27, 0x56}; /* FC 05 11 27 56 */
    
    /* Parse ID003 response */
    MESSAGE_Init(&received_id003_response_msg, PROTO_ID003, MSG_DIR_RX);
    received_id003_response_msg.opcode = id003_response_raw[2];
    
    /* Copy raw response */
    for (uint8_t i = 0; i < 5; i++)
    {
        received_id003_response_msg.raw[i] = id003_response_raw[i];
    }
    received_id003_response_msg.length = 5;
    
    /* Store response opcode for debugger */
    test_received_id003_response = id003_response_raw[2]; /* 0x11 = ID003_STATUS_IDLING */
    
    /* Update state */
    test_current_state = TEST_STATE_RECEIVED_ID003_RESPONSE;
    
    /* Breakpoint here to inspect received ID003 response */
    __NOP();
}

/**
  * @brief  Test converting and sending CCNET response
  * @retval None
  */
void PROTO_CONVERTER_Test_SendCCNETResponse(void)
{
    /* Map ID003 response to CCNET response */
    uint8_t ccnet_response_opcode = PROTO_CONVERTER_MapID003ToCCNET(test_received_id003_response);
    
    /* Create CCNET response message */
    ccnet_response_msg = MESSAGE_Create(PROTO_CCNET, MSG_DIR_RX, ccnet_response_opcode, NULL, 0);
    
    /* Copy to debugger variables */
    for (uint8_t i = 0; i < ccnet_response_msg.length; i++)
    {
        test_ccnet_response_to_send[i] = ccnet_response_msg.raw[i];
    }
    test_ccnet_response_to_send_length = ccnet_response_msg.length;
    
    
    /* Update state */
    test_current_state = TEST_STATE_SENT_CCNET_RESPONSE;
    
    /* Breakpoint here to inspect final CCNET response and verification */
    __NOP();
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Parse CCNET message from raw data
  * @param  raw_data: pointer to raw CCNET data
  * @param  length: length of raw data
  * @param  msg: pointer to message structure to fill
  * @retval None
  */
static void PROTO_CONVERTER_ParseCCNETMessage(const uint8_t* raw_data, uint8_t length, message_t* msg)
{
    /* CCNET format: [0x02][0x03][length][opcode][data][crc_lsb][crc_msb] */
    if (length >= 4)
    {
        MESSAGE_Init(msg, PROTO_CCNET, MSG_DIR_RX);
        msg->opcode = raw_data[3]; /* opcode is at index 3 */
        
        /* Copy raw data */
        for (uint8_t i = 0; i < length; i++)
        {
            msg->raw[i] = raw_data[i];
        }
        msg->length = length;
    }
}

/**
  * @brief  Extract opcode from CCNET message
  * @param  raw_data: pointer to raw CCNET data
  * @param  length: length of raw data
  * @retval CCNET opcode
  */
static uint8_t PROTO_CONVERTER_GetCCNETOpcode(const uint8_t* raw_data, uint8_t length)
{
    if (length >= 4)
    {
        return raw_data[3]; /* opcode is at index 3 */
    }
    return 0xFF; /* Invalid */
}

/**
  * @brief  Map CCNET opcode to ID003 opcode
  * @param  ccnet_opcode: CCNET opcode
  * @retval ID003 opcode
  */
static uint8_t PROTO_CONVERTER_MapCCNETToID003(uint8_t ccnet_opcode)
{
    return PROTO_MapOpcode(PROTO_CCNET, PROTO_ID003, ccnet_opcode, MAP_CCNET_TO_ID003);
}

/**
  * @brief  Map ID003 status to CCNET status
  * @param  id003_status: ID003 status
  * @retval CCNET status
  */
static uint8_t PROTO_CONVERTER_MapID003ToCCNET(uint8_t id003_status)
{
    return PROTO_MapOpcode(PROTO_ID003, PROTO_CCNET, id003_status, MAP_ID003_TO_CCNET);
}
