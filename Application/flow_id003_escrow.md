# ID003 Escrow Flow

## ID003 Command Reference

### ID003 Commands (Proxy → Validator)

| Command | Code | Description |
|---------|------|-------------|
| STATUS_REQ | 0x11 | Request current status |
| STACK_1 | 0x41 | Stack bill (option 1) |
| STACK_2 | 0x42 | Stack bill (option 2) |
| RETURN | 0x43 | Return bill to customer |
| HOLD | 0x44 | Hold bill in current position |

### ID003 Status Responses (Validator → Proxy)

| Status | Code | Data | Description |
|--------|------|------|-------------|
| IDLING | 0x11 | - | Ready, no bill |
| ACCEPTING | 0x12 | - | Validating bill |
| ESCROW | 0x13 | Denom (0x61-0x68) | Bill in escrow |
| STACKING | 0x14 | - | Moving bill to stacker |
| VEND_VALID | 0x15 | Denom (0x61-0x68) | Bill accepted |
| STACKED | 0x16 | - | Bill in stacker |
| REJECTING | 0x17 | Reason (0x71-0x7F) | Bill rejected |
| RETURNING | 0x18 | - | Returning bill |
| HOLDING | 0x19 | - | Bill on hold |
|ACK|||

## CCNET Command Reference

### CCNET Commands (Controller → Proxy)

| Command | Code | Description |
|---------|------|-------------|
| POLL | 0x33 | Request status |
| STACK | 0x35 | Accept bill in escrow |
| RETURN | 0x36 | Return bill in escrow |
| HOLD | 0x38 | Hold bill |

### CCNET Status Responses (Proxy → Controller)

| Status | Code | Data | Description |
|--------|------|------|-------------|
| IDLING | 0x14 | - | Ready, no bill |
| ACCEPTING | 0x15 | - | Validating bill |
| ESCROW_POSITION | 0x80 | Denom in bits of 3 bytes| Bill in escrow |
| STACKING | 0x17 | - | Moving to stacker |
| BILL_STACKED | 0x81 | Denom | Bill accepted |
| REJECTING | 0x1C | Reason | Bill rejected |
| RETURNING | 0x18 | - | Returning bill |
| HOLDING | 0x1A | - | Bill on hold |

## ID003 Escrow Happy Flow

| Proxy | Validator Response | State | Timeout |
|------------|-------------------|-------|---------|
| POLL | ENABLE(IDLING) | On standby | |
| POLL | ACCEPTING | Bill insertion | |
| POLL | ACCEPTING | Discriminating bill | <3s to next poll |
| POLL | ESCROW + 0x6x | Escrow | <10s to stack/return/hold|
| **STACK_1** | ACK | to stack | |
| POLL | STACKING | Conveying bill | |
| POLL | VEND_VALID + 0x6x | Bill processed | |
|**ACK**|no resp||No progress until controller sends ACK|
| POLL | STACKED | Stacking | |
| POLL | ENABLE(IDLING) | On standby | |



## CCNET Escrow Happy Flow

| Controller  | Proxy Response | State |
|------------------|--------------|-------|
| POLL | IDLING | idle |
| POLL | ACCEPTING | accepting |
| POLL | ESCROW_POSITION + denom | escrow |
| STACK | ACK | stacking |
| POLL | STACKING | stacking |
| POLL | BILL_STACKED + denom | complete |
| POLL | IDLING | idle |

## Combined ID003/CCNET Escrow Happy Flow

| Controller |escrow state |Proxy Resp | Proxy Req | Validator Response | escrow_state | Notes |
|------------|---|---------|-----------|-------------------|--------------|---|
||| | POLL | ENABLE(IDLING) | idle | |
|| | | POLL | ACCEPTING |  | remains idle|
| POLL|idle| ACCEPTING|  |  |  | |
| | | |POLL | ACCEPTING | | |
| | | |POLL | ESCROW + byte | in_escrow |10s max|
| | | |POLL | ESCROW + byte | in_escrow |10s max|
|POLL|in_escrow |ESCROW_POSITION + bits |||in_escrow||
|**STACK**|in_escrow  ||||||
|| | | **STACK_1** | ACK | in_stack| |
||| ACK ||| |CCNET ack after downstream ACK|
|| | | POLL | STACKING | stacking | |
| POLL|in_stack/ stacking| STACKING|  |  |  | |
|| | | POLL | VEND_VALID + byte|**stacking**| automate action to send ACK if state == STACKED and right denom byte | 
|| | |**ACK**|no resp|stacking||
|| | | POLL | STACKED |stacked  | |
| POLL|stacked| STACKED + bits|  |  |idle  | |
|| | | POLL | ENABLE(IDLING) |  | |


# ID003 ESCROW Flows
### timeout after entering bill
`IDLING -> ACCEPTING -> 10s ESCROW+denom -> (as long as bill is not taken out) REJECTING + 0x79 -> IDLING`

### timeout after entering bill when not polling
only 3s and then returning (no polling results)

### returning
`ESCROW -> (as long as bill is not taken out) RETURNING -> IDLING`

### stacking first part
`poll->ESCROW,  stack-1->ACK,  2s poll->STACKING,  poll->VEND VALID`

### VEND VALID acknowledged
`poll->VEND VALID,  ACK->(no response),  poll->STACKED, poll->IDLING`

### stacking, not responding to VEND VALID
sending ACK after one minute did not return anything. No response to poll. RESET worked


