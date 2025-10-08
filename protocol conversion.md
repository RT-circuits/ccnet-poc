## Spec
- CCNET supported opcodes: 
    - RESET (0x30)
    - GET STATUS (0x31) 
    - POLL (0x33)
    - ENABLE BILL TYPES (0x34)
    - STACK (0x35)
    - RETURN (0x36)
    - IDENTIFICATION (0x37)
    - GET BILL TABLE (0x41)
- ID003 supported opcodes:
    - Reset, Status, BillType, Escrow, Stack, Return


## Terms
- CCNET
    - req: controller to periph
    - resp: opposite

## CCNET Frame
- requests: command/data
- response: ack, nack, ILLEGAL_CMD, or data

## Illegal command, CRC error
- CRC invalid -> resp NACK
- Illegal command [check what are criteria]
    - command not in CCNET support opcodes
    - The IDENTIFICATION, GET BILL TABLE, (..) commands should be sent by the Controller when Bill Validator is in the following states: Power up, Initialize, one of the Failure states (41H-47H) or Unit Disabled. Otherwise an ILLEGAL COMMAND response will be returned.
    - STACK or RETURN when no bill in escrow

## Timeouts
- 10s in escrow without instruction
    ==> send 

## Regular flows
- req POLL, resp: 1 or 2 bytes
    - simple 1 byte response:
        - resp 10-1B (regulars) + 41-46 (extended regulars) + 80-82 (bill in escrow), req ACK (ignore)
    - 2 byte returns for:
        - rejection 1C xx
        - motor failure 47 xx
        - bill in escrow 80 + bill type 0-23
    ==> create us status from mapping ds status
        - already known -> just map
        - unknown or expired -> ds POLL

- req STATUS, resp: 3 bytes with enabled bill type follwed by 3 bytes high security bill types.
    - first 3 bytes: 0-23 (bit0 of 3rd byte is bill type 0, bit7 of first byte is 23). Enabled probably 1
    - 80 00 7F after ENABLE BILL TYPES FF FF FF xx xx xx
    - next 3: security. Not support
    - 00 00 00 after ENABLE BILL TYPES as above. SET SECURITY is illegal command
    ==> [retrieve ds bill types and only respond then]

- req ENABLE BILL TYPES + 6 bytes, resp: [ACK/NACK?]
    - first 3 bytes: bills. enable: 1
    - last 3 bytes: escrow or not
    ==> set ds bill types per configuration bill table

- req STACK
    - no data. resp ACK
    - [find out wheterh ds is stacked. Through normal loop, or poll extra? ]
    ==> if stacked successful (check ds), first us POLL should return STACKED

- req RETURN
    - as STACK
    ==> return RETURNED to first us req POLL

- req IDENTIFICATION + 34 bytes
    - first 27 ASCII (1-15 model, 16-27 serial nr)
    - next 7 binary asset number (unique nr)
    ==> [get from ID003]

- req GET BILL TABLE, resp 120 bytes
    - 5 bytes per bill type, 24 in total
    - rather cryptic

## Rejection flows
- req POLL, resp 1C, [reject reasons], req ACK 

## Failure flows
- req POLL, resp 47, [motor failure reasons], req ACK


## Automated actions
- when ENABLE BILL TYPES is not escrow:
    - ds resp to stack the bill
    - us resp that bill is stacked (when stacked)

ID003: VEND VALID is similar to BILL STACKED + xx. This requires an ACK (!!) STACKED follows after and is then equal to IDLING

