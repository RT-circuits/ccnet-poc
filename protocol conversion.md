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
- at startup set ID003 max bills in escrow to 1

ID003: VEND VALID is similar to BILL STACKED + xx. This requires an ACK (!!) STACKED follows after and is then equal to IDLING





# Bill types
## CCNET
- GET BILL TABLE returns validator characteristics
    - returns 24x 5 bytes. 
    - format: V CCC S, Value <10, currency in ASCII, trailing scaling zero's 
    `['05', '45', '55', '52', '00']` = 5 EUR times 10^0
    - 5 to 500 returned by CashCode FLS. 
    ==> config menu: specify denominators and currency
- ENABLE BILL TYPES
    - 3 bytes, bit0 is first bill type (e.g. 5eur)
- POLL response ESCROW and BILL STACKED
    - 1 byte with denomination nr (0=5eur, 1=10eur, 2=20eur, 3=50eur)
    - ESCROW 0x80 0x03 verified for 50eur
==> config menu: specify denominators and currency

## ID003
- bill table: CURRENCY ASSIGN DATA (0x8A)
```
OUT: FC 05 8A 7D 7C CURRENCY_ASSIGN_REQ
 IN: FC 25 8A 
 61 00 00 00 
 62 E0 05 00 
 63 E0 0A 00 
 64 E0 14 00 
 65 E0 32 00 
 66 E0 0A 01 
 67 E0 14 01 
 68 E0 32 01 
 05 C1
```
- ENABLE BILL TYPES
    - 
    - needs to be followed by INIBIBIT 0x00 (0x00 turns inhibit off. 0x01 turns it on)
    - follow up by 0xCD 01: ESCROW LIMIT to 1. ID003 can escrow multiple bills. CCNET cannot

==> config menu started: set ID003 to inhibit


