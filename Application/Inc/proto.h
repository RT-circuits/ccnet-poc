/**
  ******************************************************************************
  * @file           : proto.h
  * @brief          : Protocol module header file
  *                   Contains protocol definitions and prototypes
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __PROTO_H
#define __PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "proto_types.h"

/* Forward declarations ------------------------------------------------------*/
typedef struct message_t message_t;

/* Exported types ------------------------------------------------------------*/






/* Exported constants --------------------------------------------------------*/


//###########################################################################################
//# ID003 Transmit: Opcodes
//###########################################################################################
/* ID003 Operation Commands */
#define ID003_STATUS_REQ           0x11
#define ID003_RESET                0x40
#define ID003_STACK_1              0x41
#define ID003_STACK_2              0x42
#define ID003_RETURN               0x43
#define ID003_HOLD                 0x44
#define ID003_WAIT                 0x45

/* ID003 Setting Commands */
#define ID003_ENABLE               0xC0
#define ID003_SECURITY             0xC1
#define ID003_COMM_MODE            0xC2
#define ID003_INHIBIT              0xC3
#define ID003_DIRECTION            0xC4
#define ID003_OPT_FUNC             0xC5

/* ID003 Status Requests */
#define ID003_ENABLE_REQ           0x80
#define ID003_SECURITY_REQ         0x81
#define ID003_COMM_MODE_REQ        0x82
#define ID003_INHIBIT_REQ          0x83
#define ID003_DIRECTION_REQ        0x84
#define ID003_OPT_FUNC_REQ         0x85
#define ID003_VERSION_REQ          0x88
#define ID003_BOOT_VERSION_REQ     0x89
#define ID003_CURRENCY_ASSIGN_REQ  0x8A

//###########################################################################################
//# ID003 Receive: Status Responses
//###########################################################################################
/* ID003 Status Responses */
#define ID003_STATUS_ACK                           0x50
#define ID003_STATUS_IDLING                        0x11
#define ID003_STATUS_ACCEPTING                     0x12
#define ID003_STATUS_ESCROW                        0x13
#define ID003_STATUS_STACKING                      0x14
#define ID003_STATUS_VEND_VALID                    0x15
#define ID003_STATUS_STACKED                       0x16
#define ID003_STATUS_REJECTING                     0x17
#define ID003_STATUS_RETURNING                     0x18
#define ID003_STATUS_HOLDING                       0x19
#define ID003_STATUS_DISABLE_INHIBIT               0x1A
#define ID003_STATUS_INITIALIZE                    0x1B
#define ID003_STATUS_POWER_UP                      0x40
#define ID003_STATUS_POWER_UP_BIA                  0x41
#define ID003_STATUS_POWER_UP_BIS                  0x42
#define ID003_STATUS_STACKER_FULL                  0x43
#define ID003_STATUS_STACKER_OPEN                  0x44
#define ID003_STATUS_ACCEPTOR_JAM                  0x45
#define ID003_STATUS_STACKER_JAM                   0x46
#define ID003_STATUS_PAUSE                         0x47
#define ID003_STATUS_CHEATED                       0x48
#define ID003_STATUS_FAILURE                       0x49
#define ID003_STATUS_COMM_ERROR                    0x4A
#define ID003_STATUS_INVALID_COMMAND               0x4B

//###########################################################################################
//# CCNET Transmit: Opcodes
//###########################################################################################
/* CCNET Commands */
#define CCNET_ACK                  0x00
#define CCNET_NAK                  0xFF
#define CCNET_RESET                0x30
#define CCNET_STATUS_REQUEST       0x31
#define CCNET_SET_SECURITY         0x32
#define CCNET_POLL                 0x33
#define CCNET_ENABLE_BILL_TYPES    0x34
#define CCNET_STACK                0x35
#define CCNET_RETURN               0x36
#define CCNET_IDENTIFICATION       0x37
#define CCNET_HOLD                 0x38
#define CCNET_SET_BAR_PARAMETERS   0x39
#define CCNET_BILL_TABLE           0x41
#define CCNET_REQUEST_STATISTICS   0x60

//###########################################################################################
//# CCNET Receive: Status Responses
//###########################################################################################

/* CCNET Status Responses */
#define CCNET_STATUS_ACK                           0x00 // for parsing in message.c only
#define CCNET_STATUS_NAK                           0xFF // for parsing in message.c only
#define CCNET_STATUS_POWER_UP                      0x10
#define CCNET_STATUS_POWER_UP_BILL_IN_VALIDATOR    0x11
#define CCNET_STATUS_POWER_UP_BILL_IN_STACKER      0x12
#define CCNET_STATUS_INITIALIZE                    0x13
#define CCNET_STATUS_IDLING                        0x14
#define CCNET_STATUS_ACCEPTING                     0x15
#define CCNET_STATUS_STACKING                      0x17
#define CCNET_STATUS_RETURNING                     0x18
#define CCNET_STATUS_UNIT_DISABLED                 0x19
#define CCNET_STATUS_HOLDING                       0x1A
#define CCNET_STATUS_DEVICE_BUSY                   0x1B
#define CCNET_STATUS_REJECTING                     0x1C
#define CCNET_STATUS_DROP_CASSETTE_FULL            0x41
#define CCNET_STATUS_DROP_CASSETTE_OUT_POSITION    0x42
#define CCNET_STATUS_VALIDATOR_JAMMED              0x43
#define CCNET_STATUS_DROP_CASSETTE_JAMMED          0x44
#define CCNET_STATUS_CHEATED                       0x45
#define CCNET_STATUS_PAUSE                         0x46
#define CCNET_STATUS_MOTOR_FAILURE                 0x47
#define CCNET_STATUS_ESCROW_POSITION               0x80
#define CCNET_STATUS_BILL_STACKED                  0x81
#define CCNET_STATUS_BILL_RETURNED                 0x82


/* CCNET Reject Reasons (when status = 0x1C) */
#define CCNET_REJECT_INSERTION                     0x60
#define CCNET_REJECT_MAGNETIC                      0x61
#define CCNET_REJECT_REMAINED_BILL_IN_HEAD         0x62
#define CCNET_REJECT_MULTIPLYING                   0x63
#define CCNET_REJECT_CONVEYING                     0x64
#define CCNET_REJECT_IDENTIFICATION                0x65
#define CCNET_REJECT_VERIFICATION                  0x66
#define CCNET_REJECT_OPTIC                         0x67
#define CCNET_REJECT_INHIBIT                       0x68
#define CCNET_REJECT_CAPACITY                      0x69
#define CCNET_REJECT_OPERATION                     0x6A
#define CCNET_REJECT_LENGTH                        0x6C

/* ID003 Reject Reasons mapping (see page 11/36 of ID003 protocol specification) */
extern const uint8_t id003_reject_map[];

/* CCNET Motor Failure Types (when status = 0x47) */
#define CCNET_MOTOR_FAIL_STACK_MOTOR               0x50
#define CCNET_MOTOR_FAIL_TRANSPORT_SPEED           0x51
#define CCNET_MOTOR_FAIL_TRANSPORT_MOTOR           0x52
#define CCNET_MOTOR_FAIL_ALIGNING                  0x53
#define CCNET_MOTOR_FAIL_INITIAL_CASSETTE          0x54
#define CCNET_MOTOR_FAIL_OPTIC_CANAL               0x55
#define CCNET_MOTOR_FAIL_MAGNETIC_CANAL            0x56
#define CCNET_MOTOR_FAIL_CAPACITANCE_CANAL         0x5F

extern const uint8_t id003_failure_map[];

/* Exported variables --------------------------------------------------------*/


/* Exported functions prototypes ---------------------------------------------*/
uint8_t IsSupportedCcnetCommand(uint8_t opcode);
uint8_t PROTO_IsId003StatusCode(uint8_t status_code);
void PROTO_Init(void);
void PROTO_Process(uint8_t* data, uint16_t length);
void PROTO_SendMessage(uint8_t* data, uint16_t length);

/* Functions that use message_t (declared after forward declaration) */
uint8_t PROTO_MapStatusCode(message_t* ds_msg, message_t* us_msg);

#ifdef __cplusplus
}
#endif

#endif /* __PROTO_H */
