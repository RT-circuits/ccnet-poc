/**
  ******************************************************************************
  * @file           : message_test.c
  * @brief          : Message test module implementation
  *                   Message construction test functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Message Test Suite Documentation
 * ================================
 * 
 * OVERVIEW:
 * This test suite validates message construction and manipulation functionality
 * for CCNET and ID003 bill validator protocols. Tests verify proper message
 * structure creation, data handling, and protocol-specific formatting.
 * 
 * FUNCTIONS UNDER TEST:
 * 1. void MESSAGE_Init(message_t* msg, proto_name_t protocol, uint8_t opcode)
 * 2. void MESSAGE_SetData(message_t* msg, uint8_t* data, uint8_t data_length)
 * 3. void MESSAGE_Construct(message_t* msg)
 * 
 * TEST DATA - MESSAGE CONSTRUCTION SCENARIOS:
 * ===========================================
 * 
 * Test Scenarios:
 * ---------------
 * • ID003 POLL Request: Empty data message
 * • CCNET Status Response: Empty status message  
 * • CCNET Reset Command: Message with payload data
 * • CCNET Bill Types: Message with configuration data
 * 
 * TEST CASES:
 * ===========
 * 
 * Test A: MESSAGE_Init and MESSAGE_Construct
 * -------------------------------------------
 * Purpose: Validate basic message initialization and construction
 * Method: Initialize message with protocol/opcode, construct raw message
 * Expected Results: Proper message structure with correct headers and length
 * 
 * Test B: MESSAGE_SetData and MESSAGE_Construct
 * ---------------------------------------------
 * Purpose: Validate message construction with payload data
 * Method: Set data payload, construct complete message
 * Expected Results: Message includes data payload with correct length
 * 
 * Test C: Protocol-Specific Message Construction
 * ----------------------------------------------
 * Purpose: Validate protocol-specific message formatting
 * Method: Test different protocols (CCNET, ID003) with various opcodes
 * Expected Results: Correct protocol headers and message structure
 * 
 * DEBUGGING NOTES:
 * ================
 * - Tests are designed for debugger execution with breakpoints
 * - Inspect message structure fields to validate construction
 * - Check raw message bytes for proper protocol formatting
 * - Verify message length calculations
 */

/* Includes ------------------------------------------------------------------*/
#include "message_test.h"
#include "message.h"
#include "proto.h"

/* Private variables ---------------------------------------------------------*/

/* Test data arrays */
static const uint8_t test_payload_short[] = {0x01, 0x02, 0x03};
static const uint8_t test_payload_long[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
static const uint8_t test_bill_types[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Test A: Basic message initialization and construction
  * @retval None
  */
void MESSAGE_Test_A_BasicConstruction(void)
{
    message_t msg;
    uint8_t result;
    
    // Test ID003 POLL request (empty data)
    MESSAGE_Init(&msg, PROTO_ID003, MSG_DIR_TX, ID003_STATUS_REQ);
    MESSAGE_SetData(&msg, NULL, 0);
    MESSAGE_Construct(&msg);
    result = (msg.protocol == PROTO_ID003 && msg.opcode == ID003_STATUS_REQ && msg.length > 0);
    // Expected: result = 1 (valid message construction)
    
    // Test CCNET status response (empty data)
    MESSAGE_Init(&msg, PROTO_CCNET, MSG_DIR_RX, CCNET_STATUS_IDLING);
    MESSAGE_SetData(&msg, NULL, 0);
    MESSAGE_Construct(&msg);
    result = (msg.protocol == PROTO_CCNET && msg.opcode == CCNET_STATUS_IDLING && msg.length > 0);
    // Expected: result = 1 (valid message construction)
    
    // Test CCNET reset command (empty data)
    MESSAGE_Init(&msg, PROTO_CCNET, MSG_DIR_TX, CCNET_RESET);
    MESSAGE_SetData(&msg, NULL, 0);
    MESSAGE_Construct(&msg);
    result = (msg.protocol == PROTO_CCNET && msg.opcode == CCNET_RESET && msg.length > 0);
    // Expected: result = 1 (valid message construction)
}

/**
  * @brief  Test B: Message construction with payload data
  * @retval None
  */
void MESSAGE_Test_B_DataPayload(void)
{
    message_t msg;
    uint8_t result;
    
    // Test CCNET message with short payload
    MESSAGE_Init(&msg, PROTO_CCNET, MSG_DIR_TX, CCNET_RESET);
    MESSAGE_SetData(&msg, (uint8_t*)test_payload_short, sizeof(test_payload_short));
    MESSAGE_Construct(&msg);
    result = (msg.protocol == PROTO_CCNET && msg.data_length == sizeof(test_payload_short) && msg.length > 0);
    // Expected: result = 1 (valid message with data)
    
    // Test CCNET message with long payload
    MESSAGE_Init(&msg, PROTO_CCNET, MSG_DIR_TX, CCNET_ENABLE_BILL_TYPES);
    MESSAGE_SetData(&msg, (uint8_t*)test_payload_long, sizeof(test_payload_long));
    MESSAGE_Construct(&msg);
    result = (msg.protocol == PROTO_CCNET && msg.data_length == sizeof(test_payload_long) && msg.length > 0);
    // Expected: result = 1 (valid message with data)
    
    // Test ID003 message with bill types data
    MESSAGE_Init(&msg, PROTO_ID003, MSG_DIR_TX, ID003_STATUS_REQ);
    MESSAGE_SetData(&msg, (uint8_t*)test_bill_types, sizeof(test_bill_types));
    MESSAGE_Construct(&msg);
    result = (msg.protocol == PROTO_ID003 && msg.data_length == sizeof(test_bill_types) && msg.length > 0);
    // Expected: result = 1 (valid message with data)
}

/**
  * @brief  Test C: Protocol-specific message formatting
  * @retval None
  */
void MESSAGE_Test_C_ProtocolSpecific(void)
{
    message_t msg;
    uint8_t result;
    
    // Test CCNET protocol header format
    MESSAGE_Init(&msg, PROTO_CCNET, MSG_DIR_TX, CCNET_STATUS_REQUEST);
    MESSAGE_SetData(&msg, NULL, 0);
    MESSAGE_Construct(&msg);
    result = (msg.raw[0] == 0x02 && msg.raw[1] == 0x03 && msg.protocol == PROTO_CCNET);
    // Expected: result = 1 (correct CCNET header format)
    
    // Test ID003 protocol header format
    MESSAGE_Init(&msg, PROTO_ID003, MSG_DIR_TX, ID003_STATUS_REQ);
    MESSAGE_SetData(&msg, NULL, 0);
    MESSAGE_Construct(&msg);
    result = (msg.raw[0] == 0xFC && msg.protocol == PROTO_ID003);
    // Expected: result = 1 (correct ID003 header format)
    
    // Test message length calculation
    MESSAGE_Init(&msg, PROTO_CCNET, MSG_DIR_TX, CCNET_POLL);
    MESSAGE_SetData(&msg, (uint8_t*)test_payload_short, sizeof(test_payload_short));
    MESSAGE_Construct(&msg);
    result = (msg.length == (2 + 1 + msg.data_length + 2)); // Header + opcode + data + CRC
    // Expected: result = 1 (correct length calculation)
}

/**
  * @brief  Run all message tests
  * @retval None
  */
void MESSAGE_RunAllTests(void)
{
    MESSAGE_Test_A_BasicConstruction();
    MESSAGE_Test_B_DataPayload();
    MESSAGE_Test_C_ProtocolSpecific();
}

/* Private functions ---------------------------------------------------------*/
