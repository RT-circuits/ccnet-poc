/**
  ******************************************************************************
  * @file           : crc_test.c
  * @brief          : CRC test module implementation
  *                   CRC test functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* CRC Test Suite Documentation
 * ============================
 * 
 * OVERVIEW:
 * This test suite validates the CRC calculation and appending functionality
 * for CCNET and ID003 bill validator protocols. The tests use real-world
 * message data to ensure accurate CRC implementation.
 * 
 * FUNCTIONS UNDER TEST:
 * 1. uint16_t CRC_Calculate(uint8_t* data, proto_name_t protocol, uint16_t length)
 * 2. uint16_t CRC_AppendCRC(message_t* msg, uint16_t pos)
 * 3. crc_result_t CRC_Validate(message_t* msg)
 * 
 * TEST DATA - REAL PROTOCOL MESSAGES:
 * ===================================
 * 
 * CCNET Protocol Messages:
 * ------------------------
 * • POLL Request:     02 03 06 33 DA 81
 *   - Data: 02 03 06 33
 *   - CRC: DA 81 (little-endian: 0x81DA)
 * 
 * • Idling Response:  02 03 06 14 67 D4  
 *   - Data: 02 03 06 14
 *   - CRC: 67 D4 (little-endian: 0xD467)
 * 
 * • IDENTIFICATION Response: 02 03 27 46 4C 53 2D 45 55 31 30 2D 36 39 33 36 33 39 30 37 4B 49 34 31 41 53 37 34 38 32 75 ED 8D C8 03 3F 7B 35 DA
 *   - Data: 02 03 27 46 4C 53 2D 45 55 31 30 2D 36 39 33 36 33 39 30 37 4B 49 34 31 41 53 37 34 38 32 75 ED 8D C8 03 3F 7B
 *   - CRC: 35 DA (little-endian: 0xDA35)
 * 
 * ID003 Protocol Messages:
 * -----------------------
 * • STATUS_REQ:       FC 05 11 27 56
 *   - Data: FC 05 11
 *   - CRC: 27 56 (little-endian: 0x5627)
 * 
 * • VERSION_REQ_RESP: FC 2F 88 69 28 45 55 52 35 29 31 30 30 2D 53 53 20 49 44 30 30 33 2D 30 35 56 33 30 30 2D 34 35 20 32 38 4A 55 4E 32 33 20 37 46 32 33
 *   - Data: FC 2F 88 69 28 45 55 52 35 29 31 30 30 2D 53 53 20 49 44 30 30 33 2D 30 35 56 33 30 30 2D 34 35 20 32 38 4A 55 4E 32 33 20 37 46 32
 *   - CRC: 32 33 (little-endian: 0x3237)
 * 
 * TEST CASES:
 * ===========
 * 
 * Test A: CRC_Calculate on messages WITHOUT CRC
 * --------------------------------------------
 * Purpose: Validate CRC calculation on raw message data (without CRC bytes)
 * Method: Call CRC_Calculate() on data portion only
 * Expected Results:
 *   - CCNET POLL: 0x81DA
 *   - CCNET Idling: 0xD467  
 *   - CCNET IDENTIFICATION: 0xDA35
 *   - ID003 STATUS_REQ: 0x5627
 *   - ID003 VERSION_REQ_RESP: 0x3237
 * 
 * Test B: CRC_Calculate on messages WITH CRC
 * ----------------------------------------
 * Purpose: Validate CRC calculation returns 0x0000 when CRC is correct
 * Method: Call CRC_Calculate() on complete message including CRC bytes
 * Expected Results: All should return 0x0000 (indicating valid CRC)
 * 
 * Test C: CRC_AppendCRC on messages
 * --------------------------------
 * Purpose: Validate CRC appending functionality using message structures
 * Method: Use CRC_AppendCRC() to append CRC to message data
 * Expected Results: CRC correctly appended, final CRC calculation returns 0x0000
 * 
 * Test E: CRC_Validate on messages
 * --------------------------------
 * Purpose: Validate CRC validation functionality using message structures
 * Method: Use CRC_Validate() on various message scenarios
 * Expected Results:
 *   - Valid messages: CRC_OK
 *   - Invalid/corrupted messages: CRC_NOT_OK
 *   - NULL/invalid inputs: CRC_NOT_OK
 * 
 * DEBUGGING NOTES:
 * ================
 * - Tests are designed for debugger execution with breakpoints
 * - Inspect 'crc_result' variables to validate expected values
 * - Use CRC_RunAllTests() to execute all tests sequentially
 * - Individual test functions available for focused debugging
 * 
 * PROTOCOL SUPPORT:
 * ================
 * - CCNET: Uses CRC-CCITT polynomial 0x1021, start value 0x0000
 * - ID003: Uses CRC-CCITT polynomial 0x1021, start value 0x0000  
 * - CCTALK: Placeholder for future implementation
 */

/* Includes ------------------------------------------------------------------*/
#include "crc.h"
#include "message.h"
#include "proto.h"

/* Private variables ---------------------------------------------------------*/

/* Test data arrays */
static const uint8_t ccnet_poll_request[] = {0x02, 0x03, 0x06, 0x33};
static const uint8_t ccnet_idling_response[] = {0x02, 0x03, 0x06, 0x14};
static const uint8_t ccnet_identification_response[] = {
    0x02, 0x03, 0x27, 0x46, 0x4C, 0x53, 0x2D, 0x45, 0x55, 0x31, 0x30, 0x2D, 
    0x36, 0x39, 0x33, 0x36, 0x33, 0x39, 0x30, 0x37, 0x4B, 0x49, 0x34, 0x31, 
    0x41, 0x53, 0x37, 0x34, 0x38, 0x32, 0x75, 0xED, 0x8D, 0xC8, 0x03, 0x3F, 0x7B
};

static const uint8_t id003_status_req[] = {0xFC, 0x05, 0x11};
static const uint8_t id003_version_req_resp[] = {
    0xFC, 0x2F, 0x88, 0x69, 0x28, 0x45, 0x55, 0x52, 0x35, 0x29, 0x31, 0x30, 
    0x30, 0x2D, 0x53, 0x53, 0x20, 0x49, 0x44, 0x30, 0x30, 0x33, 0x2D, 0x30, 
    0x35, 0x56, 0x33, 0x30, 0x30, 0x2D, 0x34, 0x35, 0x20, 0x32, 0x38, 0x4A, 
    0x55, 0x4E, 0x32, 0x33, 0x20, 0x37, 0x46, 0x32, 0x33
};

/**
  * @brief  Test A: CRC_Calculate on messages WITHOUT CRC
  * @retval None
  */
  void CRC_Test_A_CalculateWithoutCRC(void)
  {
      uint16_t crc_result;
      
      // Test CCNET POLL request (without CRC)
      crc_result = CRC_Calculate((uint8_t*)ccnet_poll_request, PROTO_CCNET, sizeof(ccnet_poll_request));
      // Expected: 0x81DA (from original message: 02 03 06 33 DA 81)
      (void)crc_result; // Suppress unused variable warning
      
      // Test CCNET Idling response (without CRC)  
      crc_result = CRC_Calculate((uint8_t*)ccnet_idling_response, PROTO_CCNET, sizeof(ccnet_idling_response));
      // Expected: 0xD467 (from original message: 02 03 06 14 67 D4)
      
      // Test CCNET IDENTIFICATION response (without CRC)
      crc_result = CRC_Calculate((uint8_t*)ccnet_identification_response, PROTO_CCNET, sizeof(ccnet_identification_response));
      // Expected: 0xDA35 (from original message: ... 35 DA)
      
      // Test ID003 STATUS_REQ (without CRC)
      crc_result = CRC_Calculate((uint8_t*)id003_status_req, PROTO_ID003, sizeof(id003_status_req));
      // Expected: 0x5627 (from original message: FC 05 11 27 56)
      
      // Test ID003 VERSION_REQ_RESP (without CRC)
      crc_result = CRC_Calculate((uint8_t*)id003_version_req_resp, PROTO_ID003, sizeof(id003_version_req_resp));
      // Expected: 0x3237 (from original message: ... 32 33)
  }
  
  /**
    * @brief  Test B: CRC_Calculate on messages WITH CRC
    * @retval None
    */
  void CRC_Test_B_CalculateWithCRC(void)
  {
      uint16_t crc_result;
      
      // Test CCNET POLL request (with CRC)
      uint8_t ccnet_poll_with_crc[] = {0x02, 0x03, 0x06, 0x33, 0xDA, 0x81};
      crc_result = CRC_Calculate(ccnet_poll_with_crc, PROTO_CCNET, sizeof(ccnet_poll_with_crc));
      // Expected: 0x0000 (valid CRC)
      (void)crc_result; // Suppress unused variable warning
      
      // Test CCNET Idling response (with CRC)
      uint8_t ccnet_idling_with_crc[] = {0x02, 0x03, 0x06, 0x14, 0x67, 0xD4};
      crc_result = CRC_Calculate(ccnet_idling_with_crc, PROTO_CCNET, sizeof(ccnet_idling_with_crc));
      // Expected: 0x0000 (valid CRC)
      
      // Test ID003 STATUS_REQ (with CRC)
      uint8_t id003_status_with_crc[] = {0xFC, 0x05, 0x11, 0x27, 0x56};
      crc_result = CRC_Calculate(id003_status_with_crc, PROTO_ID003, sizeof(id003_status_with_crc));
      // Expected: 0x0000 (valid CRC)
  }
  
  /**
    * @brief  Test C: CRC_AppendCRC on messages
    * @retval None
    */
  void CRC_Test_C_AppendCRC(void)
  {
      message_t msg;
      uint16_t pos;
      uint16_t crc_result;
      
      // Test CCNET POLL request
      MESSAGE_Init(&msg, PROTO_CCNET, MSG_DIR_TX);
      msg.opcode = 0x33;
      msg.length = sizeof(ccnet_poll_request);
      for(int i = 0; i < msg.length; i++) {
          msg.raw[i] = ccnet_poll_request[i];
      }
      
      pos = CRC_AppendCRC(&msg, msg.length);
      // Validate CRC was appended correctly
      crc_result = CRC_Calculate(msg.raw, msg.protocol, pos);
      // Expected: 0x0000 (valid CRC)
      (void)crc_result; // Suppress unused variable warning
      
      // Test ID003 STATUS_REQ
      MESSAGE_Init(&msg, PROTO_ID003, MSG_DIR_TX);
      msg.opcode = 0x11;
      msg.length = sizeof(id003_status_req);
      for(int i = 0; i < msg.length; i++) {
          msg.raw[i] = id003_status_req[i];
      }
      
      pos = CRC_AppendCRC(&msg, msg.length);
      // Validate CRC was appended correctly
      crc_result = CRC_Calculate(msg.raw, msg.protocol, pos);
      // Expected: 0x0000 (valid CRC)
  }
  
  
/**
  * @brief  Test E: CRC_Validate on messages
  * @retval None
  */
void CRC_Test_E_ValidateCRC(void)
{
    message_t msg;
    crc_result_t output;
    uint8_t result;
    (void)result; // Suppress unused variable warning
    
    // Test CCNET POLL request (with valid CRC)
    MESSAGE_Init(&msg, PROTO_CCNET, MSG_DIR_TX);
    msg.opcode = 0x33;
    uint8_t ccnet_poll_with_crc[] = {0x02, 0x03, 0x06, 0x33, 0xDA, 0x81};
    msg.length = sizeof(ccnet_poll_with_crc);
    for(int i = 0; i < msg.length; i++) {
        msg.raw[i] = ccnet_poll_with_crc[i];
    }
    output = CRC_Validate(&msg);
    result = (output == CRC_OK);
    // Expected: result = 1 (CRC_OK)
    
    // Test CCNET Idling response (with valid CRC)
    MESSAGE_Init(&msg, PROTO_CCNET, MSG_DIR_RX);
    msg.opcode = 0x14;
    uint8_t ccnet_idling_with_crc[] = {0x02, 0x03, 0x06, 0x14, 0x67, 0xD4};
    msg.length = sizeof(ccnet_idling_with_crc);
    for(int i = 0; i < msg.length; i++) {
        msg.raw[i] = ccnet_idling_with_crc[i];
    }
    output = CRC_Validate(&msg);
    result = (output == CRC_OK);
    // Expected: result = 1 (CRC_OK)
    
    // Test ID003 STATUS_REQ (with valid CRC)
    MESSAGE_Init(&msg, PROTO_ID003, MSG_DIR_TX);
    msg.opcode = 0x11;
    uint8_t id003_status_with_crc[] = {0xFC, 0x05, 0x11, 0x27, 0x56};
    msg.length = sizeof(id003_status_with_crc);
    for(int i = 0; i < msg.length; i++) {
        msg.raw[i] = id003_status_with_crc[i];
    }
    output = CRC_Validate(&msg);
    result = (output == CRC_OK);
    // Expected: result = 1 (CRC_OK)
    
    // Test CCNET POLL request (with invalid CRC - corrupted)
    MESSAGE_Init(&msg, PROTO_CCNET, MSG_DIR_TX);
    msg.opcode = 0x33;
    uint8_t ccnet_poll_corrupted[] = {0x02, 0x03, 0x06, 0x33, 0xDA, 0x82}; // Changed last byte
    msg.length = sizeof(ccnet_poll_corrupted);
    for(int i = 0; i < msg.length; i++) {
        msg.raw[i] = ccnet_poll_corrupted[i];
    }
    output = CRC_Validate(&msg);
    result = (output == CRC_OK);
    // Expected: result = 0 (CRC_NOT_OK)
    
    // Test with NULL message
    output = CRC_Validate(NULL);
    result = (output == CRC_OK);
    // Expected: result = 0 (CRC_NOT_OK)
    
    // Test with zero length message
    MESSAGE_Init(&msg, PROTO_CCNET, MSG_DIR_TX);
    msg.opcode = 0x33;
    msg.length = 0;
    output = CRC_Validate(&msg);
    result = (output == CRC_OK);
    // Expected: result = 0 (CRC_NOT_OK)
}

/**
  * @brief  Run all CRC tests
  * @retval None
  */
void CRC_RunAllTests(void)
{
    CRC_Test_A_CalculateWithoutCRC();
    CRC_Test_B_CalculateWithCRC();
    CRC_Test_C_AppendCRC();
    CRC_Test_E_ValidateCRC();
}
  