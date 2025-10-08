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
#include "message.h"
#include "utils.h"

/* Private variables ---------------------------------------------------------*/
static proto_name_t protocol;


const uint16_t id003_status_lut[0x4B] = {
    [ID003_STATUS_IDLING]                    = CCNET_STATUS_IDLING,                    /* ID003: ENABLE (IDLING), CCNET: Idling */
    [ID003_STATUS_ACCEPTING]                 = CCNET_STATUS_ACCEPTING,                 /* ID003: ACCEPTING, CCNET: Accepting */
    [ID003_STATUS_ESCROW]                    = 0xFFFF,                                 /* special: ESCROW, CCNET: Escrow position */
    [ID003_STATUS_STACKING]                  = CCNET_STATUS_STACKING,                  /* ID003: STACKING, CCNET: Stacking */
    [ID003_STATUS_VEND_VALID]                = 0xFFFF,                                 /* special: VEND VALID */
    [ID003_STATUS_STACKED]                   = CCNET_STATUS_IDLING,                    /* ID003: STACKED, CCNET: Idling */
    [ID003_STATUS_REJECTING]                 = 0xFFFF,                                 /* special: REJECTING, CCNET: Rejecting */
    [ID003_STATUS_RETURNING]                 = CCNET_STATUS_RETURNING,                 /* ID003: RETURNING, CCNET: Returning */
    [ID003_STATUS_HOLDING]                   = CCNET_STATUS_HOLDING,                   /* ID003: HOLDING, CCNET: Holding */
    [ID003_STATUS_DISABLE_INHIBIT]           = CCNET_STATUS_UNIT_DISABLED,             /* ID003: DISABLE (INHIBIT), CCNET: Unit Disabled */
    [ID003_STATUS_INITIALIZE]                = CCNET_STATUS_INITIALIZE,                /* ID003: INITIALIZE, CCNET: Initialize */
    [ID003_STATUS_POWER_UP]                  = CCNET_STATUS_POWER_UP,                  /* ID003: POWER UP, CCNET: Power Up */
    [ID003_STATUS_POWER_UP_BIA]              = CCNET_STATUS_POWER_UP_BILL_IN_VALIDATOR, /* ID003: POWER UP WITH BILL IN ACCEPTOR, CCNET: Power Up with Bill in Validator */
    [ID003_STATUS_POWER_UP_BIS]              = CCNET_STATUS_POWER_UP_BILL_IN_STACKER,  /* ID003: POWER UP WITH BILL IN STACKER, CCNET: Power Up with Bill in Stacker */
    [ID003_STATUS_STACKER_FULL]              = CCNET_STATUS_DROP_CASSETTE_FULL,        /* ID003: STACKER FULL, CCNET: Drop Cassette Full */
    [ID003_STATUS_STACKER_OPEN]              = CCNET_STATUS_DROP_CASSETTE_OUT_POSITION, /* ID003: STACKER OPEN, CCNET: Drop Cassette out of position */
    [ID003_STATUS_ACCEPTOR_JAM]              = CCNET_STATUS_VALIDATOR_JAMMED,          /* ID003: JAM IN ACCEPTOR, CCNET: Validator Jammed */
    [ID003_STATUS_STACKER_JAM]               = CCNET_STATUS_DROP_CASSETTE_JAMMED,      /* ID003: JAM IN STACKER, CCNET: Drop Cassette Jammed */
    [ID003_STATUS_PAUSE]                     = CCNET_STATUS_PAUSE,                     /* ID003: PAUSE, CCNET: Pause */
    [ID003_STATUS_CHEATED]                   = CCNET_STATUS_CHEATED,                   /* ID003: CHEATED, CCNET: Cheated */
    [ID003_STATUS_FAILURE]                   = 0xFFFF,                                 /* special: FAILURE, CCNET: Generic Failure codes */
    [ID003_STATUS_COMM_ERROR]                = 0xFFFF,                                 /* special: COMMUNICATION ERROR, CCNET: Generic Failure codes (Stack Motor Failure) */
};

/* ID003 Reject Reasons mapping (see page 11/36 of ID003 protocol specification) */
const uint8_t id003_reject_map[] = {
    [0x71] = CCNET_REJECT_INSERTION,                    /* Insertion error */
    [0x72] = CCNET_REJECT_MAGNETIC,                     /* Magnetic pattern error */
    [0x73] = CCNET_REJECT_REMAINED_BILL_IN_HEAD,        /* Residual bills at head */
    [0x74] = CCNET_REJECT_OPTIC,                        /* Calibration/Magnification error */
    [0x75] = CCNET_REJECT_CONVEYING,                    /* Conveying error */
    [0x76] = CCNET_REJECT_IDENTIFICATION,               /* Discrimination error */
    [0x77] = CCNET_REJECT_VERIFICATION,                 /* Photo pattern error (1) */
    [0x78] = CCNET_REJECT_OPTIC,                        /* Photo level error */
    [0x79] = CCNET_REJECT_OPERATION,                      /* Inhibit, insterion direction, denomination, no answer to ESCROW */
    [0x7B] = CCNET_REJECT_OPERATION,                    /* Operation error */
    [0x7C] = CCNET_REJECT_REMAINED_BILL_IN_HEAD,        /* Residual at stacker (map to same) */
    [0x7D] = CCNET_REJECT_LENGTH,                       /* Length error */
    [0x7E] = CCNET_REJECT_VERIFICATION,                 /* Photo pattern error (2) */
    [0x7F] = CCNET_REJECT_VERIFICATION                  /* True bill feature error */
};

/* ID003 Failure Reasons mapping */
const uint8_t id003_failure_map[] = {
    [0xA2] = CCNET_MOTOR_FAIL_STACK_MOTOR,                    /* Stack motor failure */
    [0xA5] = CCNET_MOTOR_FAIL_TRANSPORT_SPEED,                /* Transport (feed) motor speed failure */
    [0xA6] = CCNET_MOTOR_FAIL_TRANSPORT_MOTOR,                /* Transport (feed) motor failure */
    [0xA8] = CCNET_MOTOR_FAIL_TRANSPORT_MOTOR,                /* Solenoid Failure */
    [0xA9] = CCNET_MOTOR_FAIL_STACK_MOTOR,                    /* PrintedBoard Unit failure */
    [0xAB] = CCNET_MOTOR_FAIL_INITIAL_CASSETTE,               /* Cash box not ready */
    [0xAF] = CCNET_MOTOR_FAIL_ALIGNING,                       /* Validator head remove */
    [0xB0] = CCNET_MOTOR_FAIL_TRANSPORT_MOTOR,                /* BOOT ROM failure */
    [0xB1] = CCNET_MOTOR_FAIL_TRANSPORT_MOTOR,                /* External ROM failure */
    [0xB2] = CCNET_MOTOR_FAIL_TRANSPORT_MOTOR,                /* RAM failure */
    [0xB3] = CCNET_MOTOR_FAIL_TRANSPORT_MOTOR                 /* External ROM writing failure */
};




/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

void PROTO_Init(void)
{
    // Force compiler to include the array in memory
    volatile uint8_t test = id003_failure_map[0xA2];
    (void)test; // Suppress unused variable warning
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
  * @brief  Map downstream status code to upstream status code with optional data
  * @param  ds_msg: Pointer to downstream message containing status and data
  * @param  us_msg: Pointer to upstream message to populate with mapped status and data
  * @retval uint8_t: 0 on success, non-zero on error
  * @note   Maps ID003 status codes to CCNET status codes using lookup tables.
  *         Handles special cases like ESCROW, REJECTING, and FAILURE with data mapping.
  *         Provides default values for unknown reject/failure codes.
  */
uint8_t PROTO_MapStatusCode(message_t* ds_msg, message_t* us_msg)
{
    /* downstream status code and data */
    uint8_t ds_status = ds_msg->opcode;
    uint8_t *ds_data = ds_msg->data;
    uint16_t ds_data_len = ds_msg->data_length;

    /* upstream status code and data */
    uint16_t  us_status = 0;
    uint8_t  us_data[128];
    uint16_t us_data_len = 0;


    switch (ds_msg->protocol)
    {
        case PROTO_ID003:
            us_status = id003_status_lut[ds_status];

            if (us_status == 0xFFFF)
            {
                switch (ds_status)
                {
                    case ID003_STATUS_ESCROW:
                        us_status = CCNET_STATUS_ESCROW_POSITION;
                        break;
                    case ID003_STATUS_REJECTING:
                        us_status = CCNET_STATUS_REJECTING;
                        if (ds_data_len > 0)
                        {
                            us_data_len = 1;
                            uint8_t reject_code = ds_data[0];
                            uint8_t mapped_value = id003_reject_map[reject_code];
                            
                            if (mapped_value != 0) // Non-zero means it was defined in the array
                            {
                                us_data[0] = mapped_value;
                            }
                            else
                            {
                                // Default value for unknown reject codes
                                us_data[0] = CCNET_REJECT_VERIFICATION;
                            }
                        }
                        else
                        {
                            us_data[0] = CCNET_REJECT_OPERATION;
                        }
                        break;
                    case ID003_STATUS_VEND_VALID:
                        us_status = CCNET_STATUS_BILL_STACKED;
                        break;
                    case ID003_STATUS_FAILURE:
                        us_status = CCNET_STATUS_MOTOR_FAILURE;
                        if (ds_data_len > 0)
                        {
                            uint8_t failure_code = ds_data[0];
                            uint8_t mapped_value = id003_failure_map[failure_code];
                            
                            if (mapped_value != 0) // Non-zero means it was defined in the array
                            {
                                us_data[0] = mapped_value;
                            }
                            else
                            {
                                // Default value for unknown failure codes
                                us_data[0] = CCNET_MOTOR_FAIL_TRANSPORT_MOTOR;
                            }
                            us_data_len = 1;
                        }
                        break;
                    case ID003_STATUS_COMM_ERROR:
                    /* two cases: CRC error (should just resend last req) or ILLEGAL COMMAND *
                    *  no additional byte indicating which 
                    *  optional: assume CRC with the first ID003_STATUS_COMM_ERROR, resend last request 
                    *  if still error, assume ILLEGAL COMMAND */
                        us_status = ID003_STATUS_INVALID_COMMAND;
                        us_data_len = 0;
                        break;
                    default:
                        us_status = 0xFFFF;
                        break;
                }
            }
            break; /* end PROTO_ID003 case */
            
        case PROTO_CCTALK:
            /* TODO: implement CCTALK status code mapping */
            break; /* end PROTO_CCTALK case */
            
        case PROTO_CCNET:
            /* not possible now - upstream protocol cannot map to itself */
            break; /* end PROTO_CCNET case */
            
        default:
            /* unknown protocol */
            break; /* end default case */
    } /* end switch (ds_msg->protocol) */
    
    /* Copy data to upstream message */
    us_msg->opcode = us_status;
    utils_memcpy(us_msg->data, us_data, us_data_len);
    us_msg->data_length = us_data_len;
    return 0;
}




/* Supported CCNET commands array */
const uint8_t PROTO_SUPPORTED_COMMANDS[] = {
    CCNET_RESET,              /* 0x30 - Reset */
    CCNET_STATUS_REQUEST,     /* 0x31 - Get Status */
    CCNET_POLL,               /* 0x33 - Poll */
    CCNET_ENABLE_BILL_TYPES,  /* 0x34 - Enable Bill Types */
    CCNET_STACK,              /* 0x35 - Stack */
    CCNET_RETURN,             /* 0x36 - Return */
    CCNET_IDENTIFICATION,     /* 0x37 - Identification */
    CCNET_BILL_TABLE          /* 0x41 - Get Bill Table */
};

/**
  * @brief  Check if CCNET opcode is supported
  * @param  opcode: CCNET opcode to check
  * @retval uint8_t: 1 if supported, 0 if not supported
  */
uint8_t IsSupportedCcnetCommand(uint8_t opcode)
{
    return utils_is_member(opcode, PROTO_SUPPORTED_COMMANDS, sizeof(PROTO_SUPPORTED_COMMANDS));
}

/* Array of valid ID003 status codes */
const uint8_t PROTO_id003_status_codes[] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
     0x47, 0x48, 0x49, 0x4A, 0x4B};
     
uint8_t PROTO_IsId003StatusCode(uint8_t status_code)
{
    return utils_is_member(status_code, PROTO_id003_status_codes, sizeof(PROTO_id003_status_codes));
}
