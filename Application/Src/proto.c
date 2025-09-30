/**
  ******************************************************************************
  * @file           : proto.c
  * @brief          : Protocol module implementation
  *                   Protocol handling functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "proto.h"

/* Private variables ---------------------------------------------------------*/
static proto_name_t protocol;

/* Protocol mapping table */
const proto_mapping_t proto_mapping_table[] = {
    /* CCNET to ID003 Request Mappings */
    {PROTO_CCNET, PROTO_ID003, CCNET_POLL, ID003_STATUS_REQ, MAP_CCNET_TO_ID003},
    {PROTO_CCNET, PROTO_ID003, CCNET_RESET, ID003_RESET, MAP_CCNET_TO_ID003},
    {PROTO_CCNET, PROTO_ID003, CCNET_STATUS_REQUEST, ID003_STATUS_REQ, MAP_CCNET_TO_ID003},
    {PROTO_CCNET, PROTO_ID003, CCNET_STACK, ID003_STACK_1, MAP_CCNET_TO_ID003},
    {PROTO_CCNET, PROTO_ID003, CCNET_RETURN, ID003_RETURN, MAP_CCNET_TO_ID003},
    {PROTO_CCNET, PROTO_ID003, CCNET_HOLD, ID003_HOLD, MAP_CCNET_TO_ID003},
    {PROTO_CCNET, PROTO_ID003, CCNET_ENABLE_BILL_TYPES, ID003_ENABLE, MAP_CCNET_TO_ID003},
    
    /* ID003 to CCNET Response Mappings */
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_IDLING, CCNET_STATUS_IDLING, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_ACCEPTING, CCNET_STATUS_ACCEPTING, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_STACKING, CCNET_STATUS_STACKING, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_STACKED, CCNET_STATUS_BILL_STACKED, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_RETURNING, CCNET_STATUS_RETURNING, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_REJECTING, CCNET_STATUS_REJECTING, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_HOLDING, CCNET_STATUS_HOLDING, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_ESCROW, CCNET_STATUS_ESCROW_POSITION, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_DISABLE_INHIBIT, CCNET_STATUS_UNIT_DISABLED, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_POWER_UP, CCNET_STATUS_POWER_UP, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_STACKER_FULL, CCNET_STATUS_DROP_CASSETTE_FULL, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_ACCEPTOR_JAM, CCNET_STATUS_VALIDATOR_JAMMED, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_STACKER_JAM, CCNET_STATUS_DROP_CASSETTE_JAMMED, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_CHEATED, CCNET_STATUS_CHEATED, MAP_ID003_TO_CCNET},
    {PROTO_ID003, PROTO_CCNET, ID003_STATUS_FAILURE, CCNET_STATUS_MOTOR_FAILURE, MAP_ID003_TO_CCNET},
    
    /* CCNET to CCTALK Request Mappings (if needed) */
    {PROTO_CCNET, PROTO_CCTALK, CCNET_POLL, 0x11, MAP_CCNET_TO_CCTALK},  /* CCTALK poll equivalent */
    
    /* CCTALK to CCNET Response Mappings (if needed) */
    {PROTO_CCTALK, PROTO_CCNET, 0x11, CCNET_STATUS_IDLING, MAP_CCTALK_TO_CCNET},  /* CCTALK idle to CCNET idle */
};

/* Size of mapping table */
const uint16_t PROTO_MAPPING_TABLE_SIZE = sizeof(proto_mapping_table) / sizeof(proto_mapping_t);

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

void PROTO_Init(void)
{
    return;
}

/**
  * @brief  Process received protocol data
  * @param  data: pointer to received data
  * @param  length: data length
  * @retval None
  */
void PROTO_Process(uint8_t* data, uint16_t length)
{
    // Protocol processing implementation
    switch (protocol)
    {
        case PROTO_CCNET:
            // CCNET protocol processing
            break;
        case PROTO_CCTALK:
            // CCTALK protocol processing
            break;
        case PROTO_ID003:
            // ID003 protocol processing
            break;
        default:
            break;
    }
}

/**
  * @brief  Send protocol message
  * @param  data: pointer to data to send
  * @param  length: data length
  * @retval None
  */
void PROTO_SendMessage(uint8_t* data, uint16_t length)
{
    // Protocol message sending implementation
    // This would typically call UART_SendMessage or similar
}

/**
  * @brief  Map opcode from source protocol to target protocol
  * @param  source_protocol: source protocol type
  * @param  target_protocol: target protocol type
  * @param  source_opcode: source opcode to map
  * @param  direction: mapping direction
  * @retval Mapped opcode (0xFF if not found)
  */
uint8_t PROTO_MapOpcode(proto_name_t source_protocol, proto_name_t target_protocol, uint8_t source_opcode, proto_map_direction_t direction)
{
    const proto_mapping_t* mapping = PROTO_FindMapping(source_protocol, target_protocol, source_opcode, direction);
    
    if (mapping != NULL)
    {
        return mapping->target_opcode;
    }
    
    return 0xFF; /* Not found */
}

/**
  * @brief  Find mapping entry in mapping table
  * @param  source_protocol: source protocol type
  * @param  target_protocol: target protocol type
  * @param  source_opcode: source opcode to find
  * @param  direction: mapping direction
  * @retval Pointer to mapping entry (NULL if not found)
  */
const proto_mapping_t* PROTO_FindMapping(proto_name_t source_protocol, proto_name_t target_protocol, uint8_t source_opcode, proto_map_direction_t direction)
{
    for (uint16_t i = 0; i < PROTO_MAPPING_TABLE_SIZE; i++)
    {
        const proto_mapping_t* mapping = &proto_mapping_table[i];
        
        if (mapping->source_protocol == source_protocol &&
            mapping->target_protocol == target_protocol &&
            mapping->source_opcode == source_opcode &&
            mapping->direction == direction)
        {
            return mapping;
        }
    }
    
    return NULL; /* Not found */
}
