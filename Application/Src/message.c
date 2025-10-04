/**
  ******************************************************************************
  * @file           : message.c
  * @brief          : Message handling module implementation
  *                   Protocol message construction and management
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "message.h"
#include "crc.h"

/* Private defines -----------------------------------------------------------*/
#define MESSAGE_MAX_DATA_LENGTH 250

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void MESSAGE_SetOpcode(message_t* msg, uint8_t opcode);
static void MESSAGE_SetData(message_t* msg, uint8_t* data, uint8_t data_length);
static void MESSAGE_SetRaw(message_t* msg);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Create a complete message ready for transmission
  * @param  protocol: protocol type
  * @param  direction: message direction (TX/RX)
  * @param  opcode: command/response opcode
  * @param  data: pointer to data buffer (NULL if no data)
  * @param  data_length: length of data buffer (0 if no data)
  * @retval Complete message structure ready for transmission
  */
  message_t MESSAGE_Create(proto_name_t protocol, message_direction_t direction, uint8_t opcode, uint8_t* data, uint8_t data_length)
  {
      message_t msg;
      
      /* Initialize message */
      MESSAGE_Init(&msg, protocol, direction);
      
      /* Set opcode */
      MESSAGE_SetOpcode(&msg, opcode);
      
      /* Set data if provided */
      MESSAGE_SetData(&msg, data, data_length);
      
      /* Build complete raw message */
      MESSAGE_SetRaw(&msg);
      
      return msg;
  }
  
/**
  * @brief  Initialize message structure
  * @param  msg: pointer to message structure
  * @param  protocol: protocol type
  * @param  direction: message direction (TX/RX)
  * @retval None
  */
void MESSAGE_Init(message_t* msg, proto_name_t protocol, message_direction_t direction)
{
    msg->protocol = protocol;
    msg->direction = direction;
    msg->opcode = 0;
    msg->data_length = 0;
    msg->length = 0;
    
    /* Clear data buffer */
    for (uint16_t i = 0; i < MESSAGE_MAX_DATA_LENGTH; i++)
    {
        msg->data[i] = 0;
    }
    
    /* Clear raw buffer */
    for (uint16_t i = 0; i < 256; i++)
    {
        msg->raw[i] = 0;
    }
}

/**
  * @brief  Set message data payload (static function)
  * @param  msg: pointer to message structure
  * @param  data: pointer to data buffer
  * @param  data_length: length of data buffer
  * @retval None
  */
static void MESSAGE_SetData(message_t* msg, uint8_t* data, uint8_t data_length)
{
    if (data != NULL && data_length <= MESSAGE_MAX_DATA_LENGTH)
    {
        for (uint8_t i = 0; i < data_length; i++)
        {
            msg->data[i] = data[i];
        }
        msg->data_length = data_length;
    }
    else
    {
        msg->data_length = 0;
    }
}

/**
  * @brief  Set message opcode (static function)
  * @param  msg: pointer to message structure
  * @param  opcode: command/response opcode
  * @retval None
  */
static void MESSAGE_SetOpcode(message_t* msg, uint8_t opcode)
{
    msg->opcode = opcode;
}

/**
  * @brief  Build complete raw message from message structure (static function)
  * @param  msg: pointer to message structure
  * @retval None
  */
static void MESSAGE_SetRaw(message_t* msg)
{
    uint16_t pos = 0;
    uint8_t header_length;
    
    /* Set header bytes */
    switch (msg->protocol)
    {
        case PROTO_ID003:           
            /* Header byte */
            msg->raw[pos++] = 0xFC;
            header_length = 1;
            break;
            
        case PROTO_CCNET:
            /* Header bytes */
            msg->raw[pos++] = 0x02;
            msg->raw[pos++] = 0x03;
            header_length = 2;
            break;

        case PROTO_CCTALK:
            /* Header bytes */
            //TODO: Implement CCTALK header bytes
            header_length = 0;
            break;
    }  

    /* Set length field */
    msg->raw[pos++] = header_length + 1 + 1 + msg->data_length + 2; /* header(1 or 2) + length + opcode + data + crc */
    
    /* Set opcode */
    msg->raw[pos++] = msg->opcode;
    
    /* Add data */
    if (msg->data_length > 0)
    {
        for (uint8_t i = 0; i < msg->data_length; i++)
        {
            msg->raw[pos++] = msg->data[i];
        }
    }

    /* Add CRC */
    pos = CRC_AppendCRC(msg, pos);
    msg->length = pos;
}


/**
  * @brief  Parse raw UART data and populate message structure
  * @param  msg: pointer to message structure containing raw data (input/output)
  * @note   Populates: msg->protocol, msg->opcode, msg->data[], msg->data_length
  *         Direction is already set during message initialization
  * @retval message_parse_result_t: parsing result status
  */
message_parse_result_t MESSAGE_Parse(message_t* msg)
{
    uint16_t pos = 0;
    uint8_t header_length = 0;
    uint8_t expected_length = 0;
    
    /* Validate input parameters */
    if (msg == NULL || msg->length == 0 || msg->length > 255)
    {
        return MSG_PARSE_ERROR;
    }
    
    /* Detect protocol and validate header */
    if (msg->length >= 2 && msg->raw[0] == 0x02 && msg->raw[1] == 0x03)
    {
        msg->protocol = PROTO_CCNET;
        header_length = 2;
        pos = 2;
    }
    else if (msg->length >= 1 && msg->raw[0] == 0xFC)
    {
        msg->protocol = PROTO_ID003;
        header_length = 1;
        pos = 1;
    }
    else
    {
        return MSG_INVALID_HEADER;
    }
    
    /* Check minimum message length */
    if (msg->length < header_length + 3) /* header + length + opcode + CRC */
    {
        return MSG_INVALID_LENGTH;
    }
    
    /* Extract length field */
    expected_length = msg->raw[pos++];
    
    /* Validate length field */
    if (expected_length != msg->length)
    {
        return MSG_INVALID_LENGTH;
    }
    
    /* Extract opcode */
    msg->opcode = msg->raw[pos++];
    
    /* Direction is already set when message is created - no need to detect it here */
    
    /* Validate opcode with context */
    message_parse_result_t opcode_result = MESSAGE_ValidateOpcode(msg);
    if (opcode_result != MSG_OK)
    {
        return opcode_result;
    }
    
    /* Calculate data length */
    uint16_t crc_length = 2; /* CRC is 2 bytes */
    if (pos + crc_length > msg->length)
    {
        return MSG_INVALID_LENGTH;
    }
    
    msg->data_length = msg->length - pos - crc_length;
    
    /* Extract data payload */
    if (msg->data_length > 250)
    {
        return MSG_DATA_MISSING_FOR_OPCODE; /* Data too long */
    }
    
    for (uint8_t i = 0; i < msg->data_length; i++)
    {
        msg->data[i] = msg->raw[pos++];
    }
    
    /* Validate CRC */
    if (CRC_Validate(msg) != CRC_OK)
    {
        return MSG_CRC_INVALID;
    }
    
    return MSG_OK;
}

/**
  * @brief  Get ASCII representation of opcode for logging
  * @param  msg: pointer to message structure containing protocol, direction, and opcode
  * @retval const char*: ASCII string representation
  */
const char* MESSAGE_GetOpcodeASCII(const message_t* msg)
{
    if (msg->protocol == PROTO_CCNET)
    {
        if (msg->direction == MSG_DIR_TX)
        {
            /* CCNET Transmit Commands */
            switch (msg->opcode)
            {
                case 0x00: return "CCNET_ACK";
                case 0xFF: return "CCNET_NAK";
                case 0x30: return "CCNET_RESET";
                case 0x31: return "CCNET_STATUS_REQUEST";
                case 0x32: return "CCNET_SET_SECURITY";
                case 0x33: return "CCNET_POLL";
                case 0x34: return "CCNET_ENABLE_BILL_TYPES";
                case 0x35: return "CCNET_STACK";
                case 0x36: return "CCNET_RETURN";
                case 0x37: return "CCNET_IDENTIFICATION";
                case 0x38: return "CCNET_HOLD";
                case 0x39: return "CCNET_SET_BAR_PARAMETERS";
                case 0x41: return "CCNET_BILL_TABLE";
                case 0x60: return "CCNET_REQUEST_STATISTICS";
                default: return "CCNET_TX_UNKNOWN";
            }
        }
        else
        {
            /* CCNET Receive Status Responses */
            switch (msg->opcode)
            {
                case 0x10: return "CCNET_STATUS_POWER_UP";
                case 0x11: return "CCNET_STATUS_POWER_UP_BILL_IN_VALIDATOR";
                case 0x12: return "CCNET_STATUS_POWER_UP_BILL_IN_STACKER";
                case 0x13: return "CCNET_STATUS_INITIALIZE";
                case 0x14: return "CCNET_STATUS_IDLING";
                case 0x15: return "CCNET_STATUS_ACCEPTING";
                case 0x17: return "CCNET_STATUS_STACKING";
                case 0x18: return "CCNET_STATUS_RETURNING";
                case 0x19: return "CCNET_STATUS_UNIT_DISABLED";
                case 0x1A: return "CCNET_STATUS_HOLDING";
                case 0x1B: return "CCNET_STATUS_DEVICE_BUSY";
                case 0x1C: return "CCNET_STATUS_REJECTING";
                case 0x41: return "CCNET_STATUS_DROP_CASSETTE_FULL";
                case 0x42: return "CCNET_STATUS_DROP_CASSETTE_OUT_POSITION";
                case 0x43: return "CCNET_STATUS_VALIDATOR_JAMMED";
                case 0x44: return "CCNET_STATUS_DROP_CASSETTE_JAMMED";
                case 0x45: return "CCNET_STATUS_CHEATED";
                case 0x46: return "CCNET_STATUS_PAUSE";
                case 0x47: return "CCNET_STATUS_MOTOR_FAILURE";
                case 0x80: return "CCNET_STATUS_ESCROW_POSITION";
                case 0x81: return "CCNET_STATUS_BILL_STACKED";
                case 0x82: return "CCNET_STATUS_BILL_RETURNED";
                default: return "CCNET_RX_UNKNOWN";
            }
        }
    }
    else if (msg->protocol == PROTO_ID003)
    {
        if (msg->direction == MSG_DIR_TX)
        {
            /* ID003 Transmit Commands */
            switch (msg->opcode)
            {
                case 0x11: return "ID003_STATUS_REQ";
                case 0x40: return "ID003_RESET";
                case 0x41: return "ID003_STACK_1";
                case 0x42: return "ID003_STACK_2";
                case 0x43: return "ID003_RETURN";
                case 0x44: return "ID003_HOLD";
                case 0x45: return "ID003_WAIT";
                case 0xC0: return "ID003_ENABLE";
                case 0xC1: return "ID003_SECURITY";
                case 0xC2: return "ID003_COMM_MODE";
                case 0xC3: return "ID003_INHIBIT";
                case 0xC4: return "ID003_DIRECTION";
                case 0xC5: return "ID003_OPT_FUNC";
                case 0x80: return "ID003_ENABLE_REQ";
                case 0x81: return "ID003_SECURITY_REQ";
                case 0x82: return "ID003_COMM_MODE_REQ";
                case 0x83: return "ID003_INHIBIT_REQ";
                case 0x84: return "ID003_DIRECTION_REQ";
                case 0x85: return "ID003_OPT_FUNC_REQ";
                case 0x88: return "ID003_VERSION_REQ";
                case 0x89: return "ID003_BOOT_VERSION_REQ";
                case 0x8A: return "ID003_CURRENCY_ASSIGN_REQ";
                default: return "ID003_TX_UNKNOWN";
            }
        }
        else
        {
            /* ID003 Receive Status Responses */
            switch (msg->opcode)
            {
                case 0x50: return "ID003_STATUS_ACK";
                case 0x11: return "ID003_STATUS_IDLING";
                case 0x12: return "ID003_STATUS_ACCEPTING";
                case 0x13: return "ID003_STATUS_ESCROW";
                case 0x14: return "ID003_STATUS_STACKING";
                case 0x15: return "ID003_STATUS_VEND_VALID";
                case 0x16: return "ID003_STATUS_STACKED";
                case 0x17: return "ID003_STATUS_REJECTING";
                case 0x18: return "ID003_STATUS_RETURNING";
                case 0x19: return "ID003_STATUS_HOLDING";
                case 0x1A: return "ID003_STATUS_DISABLE_INHIBIT";
                case 0x1B: return "ID003_STATUS_INITIALIZE";
                case 0x40: return "ID003_STATUS_POWER_UP";
                case 0x41: return "ID003_STATUS_POWER_UP_BIA";
                case 0x42: return "ID003_STATUS_POWER_UP_BIS";
                case 0x43: return "ID003_STATUS_STACKER_FULL";
                case 0x44: return "ID003_STATUS_STACKER_OPEN";
                case 0x45: return "ID003_STATUS_ACCEPTOR_JAM";
                case 0x46: return "ID003_STATUS_STACKER_JAM";
                case 0x47: return "ID003_STATUS_PAUSE";
                case 0x48: return "ID003_STATUS_CHEATED";
                case 0x49: return "ID003_STATUS_FAILURE";
                case 0x4A: return "ID003_STATUS_COMM_ERROR";
                case 0x4B: return "ID003_STATUS_INVALID_COMMAND";
                default: return "ID003_RX_UNKNOWN";
            }
        }
    }
    
    return "UNKNOWN_PROTOCOL";
}

/**
  * @brief  Validate opcode for known commands
  * @param  msg: pointer to message structure containing protocol, direction, and opcode
  * @retval message_parse_result_t: validation result
  */
message_parse_result_t MESSAGE_ValidateOpcode(message_t* msg)
{
    if (msg->protocol == PROTO_CCNET)
    {
        if (msg->direction == MSG_DIR_TX)
        {
            /* CCNET Transmit Commands */
            switch (msg->opcode)
            {
                case 0x00: /* CCNET_ACK */
                case 0xFF: /* CCNET_NAK */
                case 0x30: /* CCNET_RESET */
                case 0x31: /* CCNET_STATUS_REQUEST */
                case 0x32: /* CCNET_SET_SECURITY */
                case 0x33: /* CCNET_POLL */
                case 0x34: /* CCNET_ENABLE_BILL_TYPES */
                case 0x35: /* CCNET_STACK */
                case 0x36: /* CCNET_RETURN */
                case 0x37: /* CCNET_IDENTIFICATION */
                case 0x38: /* CCNET_HOLD */
                case 0x39: /* CCNET_SET_BAR_PARAMETERS */
                case 0x41: /* CCNET_BILL_TABLE */
                case 0x60: /* CCNET_REQUEST_STATISTICS */
                    return MSG_OK;
                default:
                    return MSG_UNKNOWN_OPCODE;
            }
        }
        else
        {
            /* CCNET Receive Status Responses */
            switch (msg->opcode)
            {
                case 0x10: /* CCNET_STATUS_POWER_UP */
                case 0x11: /* CCNET_STATUS_POWER_UP_BILL_IN_VALIDATOR */
                case 0x12: /* CCNET_STATUS_POWER_UP_BILL_IN_STACKER */
                case 0x13: /* CCNET_STATUS_INITIALIZE */
                case 0x14: /* CCNET_STATUS_IDLING */
                case 0x15: /* CCNET_STATUS_ACCEPTING */
                case 0x17: /* CCNET_STATUS_STACKING */
                case 0x18: /* CCNET_STATUS_RETURNING */
                case 0x19: /* CCNET_STATUS_UNIT_DISABLED */
                case 0x1A: /* CCNET_STATUS_HOLDING */
                case 0x1B: /* CCNET_STATUS_DEVICE_BUSY */
                case 0x1C: /* CCNET_STATUS_REJECTING */
                case 0x41: /* CCNET_STATUS_DROP_CASSETTE_FULL */
                case 0x42: /* CCNET_STATUS_DROP_CASSETTE_OUT_POSITION */
                case 0x43: /* CCNET_STATUS_VALIDATOR_JAMMED */
                case 0x44: /* CCNET_STATUS_DROP_CASSETTE_JAMMED */
                case 0x45: /* CCNET_STATUS_CHEATED */
                case 0x46: /* CCNET_STATUS_PAUSE */
                case 0x47: /* CCNET_STATUS_MOTOR_FAILURE */
                case 0x80: /* CCNET_STATUS_ESCROW_POSITION */
                case 0x81: /* CCNET_STATUS_BILL_STACKED */
                case 0x82: /* CCNET_STATUS_BILL_RETURNED */
                    return MSG_OK;
                default:
                    return MSG_UNKNOWN_OPCODE;
            }
        }
    }
    else if (msg->protocol == PROTO_ID003)
    {
        if (msg->direction == MSG_DIR_TX)
        {
            /* ID003 Transmit Commands */
            switch (msg->opcode)
            {
                case 0x11: /* ID003_STATUS_REQ */
                case 0x40: /* ID003_RESET */
                case 0x41: /* ID003_STACK_1 */
                case 0x42: /* ID003_STACK_2 */
                case 0x43: /* ID003_RETURN */
                case 0x44: /* ID003_HOLD */
                case 0x45: /* ID003_WAIT */
                case 0xC0: /* ID003_ENABLE */
                case 0xC1: /* ID003_SECURITY */
                case 0xC2: /* ID003_COMM_MODE */
                case 0xC3: /* ID003_INHIBIT */
                case 0xC4: /* ID003_DIRECTION */
                case 0xC5: /* ID003_OPT_FUNC */
                case 0x80: /* ID003_ENABLE_REQ */
                case 0x81: /* ID003_SECURITY_REQ */
                case 0x82: /* ID003_COMM_MODE_REQ */
                case 0x83: /* ID003_INHIBIT_REQ */
                case 0x84: /* ID003_DIRECTION_REQ */
                case 0x85: /* ID003_OPT_FUNC_REQ */
                case 0x88: /* ID003_VERSION_REQ */
                case 0x89: /* ID003_BOOT_VERSION_REQ */
                case 0x8A: /* ID003_CURRENCY_ASSIGN_REQ */
                    return MSG_OK;
                default:
                    return MSG_UNKNOWN_OPCODE;
            }
        }
        else
        {
            /* ID003 Receive Status Responses */
            switch (msg->opcode)
            {
                case 0x50: /* ID003_STATUS_ACK */
                case 0x11: /* ID003_STATUS_IDLING */
                case 0x12: /* ID003_STATUS_ACCEPTING */
                case 0x13: /* ID003_STATUS_ESCROW */
                case 0x14: /* ID003_STATUS_STACKING */
                case 0x15: /* ID003_STATUS_VEND_VALID */
                case 0x16: /* ID003_STATUS_STACKED */
                case 0x17: /* ID003_STATUS_REJECTING */
                case 0x18: /* ID003_STATUS_RETURNING */
                case 0x19: /* ID003_STATUS_HOLDING */
                case 0x1A: /* ID003_STATUS_DISABLE_INHIBIT */
                case 0x1B: /* ID003_STATUS_INITIALIZE */
                case 0x40: /* ID003_STATUS_POWER_UP */
                case 0x41: /* ID003_STATUS_POWER_UP_BIA */
                case 0x42: /* ID003_STATUS_POWER_UP_BIS */
                case 0x43: /* ID003_STATUS_STACKER_FULL */
                case 0x44: /* ID003_STATUS_STACKER_OPEN */
                case 0x45: /* ID003_STATUS_ACCEPTOR_JAM */
                case 0x46: /* ID003_STATUS_STACKER_JAM */
                case 0x47: /* ID003_STATUS_PAUSE */
                case 0x48: /* ID003_STATUS_CHEATED */
                case 0x49: /* ID003_STATUS_FAILURE */
                case 0x4A: /* ID003_STATUS_COMM_ERROR */
                case 0x4B: /* ID003_STATUS_INVALID_COMMAND */
                    return MSG_OK;
                default:
                    return MSG_UNKNOWN_OPCODE;
            }
        }
    }
    
    return MSG_UNKNOWN_OPCODE;
}

/* Private functions ---------------------------------------------------------*/
