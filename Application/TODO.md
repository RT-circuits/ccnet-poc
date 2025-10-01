Create the following: 
- crc module
- proto module
- timer module for timeouts? Callbacks per function pointer in the calling module?

## app.c
- interface typedef
    - phy
        - baudrate/wordlength/parity/polarity(normal/inverted)/crc
        - timing parameters
        - protocol
    - crc
        - type (OTHER/CRC_CCNET/CRC_CCTALK)
        - length (8BITS/16BITS)
        - polynomial
        - start value
    - protocol
        - name (PROTO_CCNET/CCTALK/IDO03)
        - header_length
        - header_bytes (maximum 3)
        - role (BILL_VALIDATOR/CONTROLLER)

create two objects: 
    - if_cnt
    - if_ctk

- abbreviations for protocols:
    - cnt: CCNET
    - ctk: CCTALK
    - id3: ID003


with all the codes in proto.h, let me explain what I want to do:
- I emulate a CCNET bill validator. I receive requests from UART1. As such I receive an opcode and possibly some data. 
- I also emulate a ID003/CCTALK controller. As such I have to map the CCNET request to a ID003 request that I will then send to an actual ID003/CCTALK bill validator 
- The most common request I will receive is CCNET_POLL. I will translate this to ID003_STATUS_REQ and send this to the bill validator. A logical response from the bill validator would be ID003_STATUS_IDLING. I then translate this to CCNET_STATUS_IDLING and send this to UART1

TODO, quite some code already in proto.c:
-  PROTO_CONVERTER_ParseCCNETMessage(test_ccnet_message, 6, &received_ccnet_msg) 

## callbacks
make sure to clear interrupt flags. Maybe even more clean code needed. check. __HAL_TIM_CLEAR_IT(&htim17, TIM_IT_UPDATE);

##  USB
- Add USB connection state detection
Implement bidirectional communication (receive data)
Add more sophisticated message formatting
agent added 50ms delays between usb outputs. Is insane of course
for (volatile int i = 0; i < 500; i++);
```
uint8_t CDC_IsFreeToTransmit()
{
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;

  if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED){
    // device was not configured yet
    return 0;
  }
  if (hcdc->TxState != 0) {
    // busy, not ready to transmit
    return 0;
  }
  return 1;
}
```
- usb with short delay now working. Very dependent on DATA_IN from the host, so do not overdo it. Nice circular buffer in my grok conversations. 
- maybe do: write all USB traffic to circular buffer anyway. And just a 100ms timer to send it along. If there is more data, something was wrong anyway
- USB_CDC_RxHandler Call Context: INTERRUPT HANDLER
```
⚠️ CRITICAL: It's Called from Interrupt Context!
USB_CDC_RxHandler is called from the USB interrupt service routine (ISR), which means:
Interrupt Context: It runs within USB_LP_IRQHandler() → HAL_PCD_IRQHandler() → USB CDC middleware → CDC_Receive_FS() → USB_CDC_RxHandler()
```

## Config
- goal: be able to set downstream protocol to ID003/CCTALK, baud rates, parity, bill table mapping
- make a simple cli menu in new module config.c
- store settings in non-volatile memory nvm.c
- read settings at startup
Current if struct:
```
typedef struct
{
    uint32_t baudrate;              /* Communication baudrate (e.g., 9600, 115200) */
    unsigned int wordlength;        /* Data word length (e.g., WORDLENGTH_8B) */
    unsigned long parity;           /* Parity setting (e.g., UART_PARITY_NONE) */
    polarity_t uart_polarity;       /* Signal polarity (normal/inverted) */
    uint8_t crc_enabled;           /* CRC calculation enabled flag */
    uint32_t polling_period_ms;     /* Polling period in milliseconds */
    UART_HandleTypeDef* uart_handle; /* Reference to UART hardware handle */
} phy_config_t;

/**
  * @brief  Interface configuration structure
  * @note   Complete configuration for a communication interface
  */
typedef struct
{
    proto_name_t protocol;         /* Protocol type (PROTO_CCNET/CCTALK/ID003) */
    proto_role_t role;             /* Device role (ROLE_BILL_VALIDATOR/CONTROLLER) */
    phy_config_t phy;              /* Physical layer configuration */
} interface_config_t;
```
- In the CLI, display two interfaces: upstream first, then downstream
- show current values
- each interface displays:
  - protocol (upstream interface cannot be changed from CCNET)
  - baudrate
  - parity
  - polling period (upstream interface: "n/a")
  - bill table (binary, 8 bits)
- at the bottom of the two interfaces:
  - USB logging enabled/disabled
  - Protocol logging short/verbose
- Changes need to be stored in nvm at the end

## Protocol sequences ID003
- ENABLE (of bills) can be done at any state. bit 0 will automatically be set to 1
- Reset needs to be followed by INHIBIT 00. Not doing it or setting it to 01 leaves it in DISABLE/INHIBIT state
- ENABLE 01 accepts all bills. ENABLE FF none. Sets state to DISABLE even

- non ENABLED bill gives REJECTING 79 
- ENABLED 10eur gives ACCEPTING follwed by ESCROW 63. 20eur: ESCROW 64. 5eur: 62
- ESCROW timeout gives REJECTING 79 after 11s

ICB
- RESET ICB_DISABLE throws FAILURE AF
- RESET ICB_ENABLE gives FAILURE 03. Playing with these require a power cycle

## ID003 bill types
- per id003 spec does NOT reflect reality. 63H below is 5eur. JCM bill validator sends 0x62 for 5 eur
DATA  |  Denomination  |  DATA  |  Denomination
------+----------------+--------+--------------
61H   |  01            |  71H   |              
62H   |  02            |  72H   |              
63H   |  03            |  73H   |  (5EURO)     
64H   |  04            |  74H   |  (10EURO)    
65H   |  05            |  75H   |  (20EURO)    
66H   |  06            |  76H   |  (50EURO)    
67H   |  07            |  77H   |  (100EURO)   
68H   |  08            |  78H   |  (200EURO)   
69H   |  09            |  79H   |  (500EURO)   

- In 0xC0 ENABLE/DISABLE, this would NOT fit. Elmooukhtar gave me:
DATA bit  |  Data 1  
----------+----------
0         |  Reserved
1         |  5       
2         |  10      
3         |  20      
4         |  50      
5         |  100     
6         |  200     
7         |  500     

## Protocol CCNET
- GET BILLTABLE in simulator gives unknown command. Check
- bill type 0 = 5eur, 1=10eur..6=500eur hardcoded


## Non Volatile memory - COMPLETED ✅
- add two storage regions that are alternated in writing. Add sequence nr (highest nr of the two plus one)
- add 16 bit crc from crc.c (same as used for ccnet)
- in APP_Init read configuration from highest sequence nr, check CRC. If wrong, take the other one and check CRC. If also corrupt, go to default settings (uart1 9600 baud no parity, CCNET, uart2 9600 Even parity, ID003
- Updated config menu with separate "Exit and Restart" and "Save, Exit and Restart" options
- NVM module fully integrated with config menu for save/load operations
- Connfiguration data:
```
Bank Address (BANK1: 0x0801F000, BANK2: F800):
┌─────────────────────────────────────┐
│ magic (4 bytes)      = 0x12345678   │ ← Header
│ version (4 bytes)    = 1            │ ← Header  
│ sequence (4 bytes)   = 1,2,3,4...   │ ← Heade
│ config_size (4 bytes)= 82           │ ← Header
├─────────────────────────────────────┤
│ config_data[82]                     │ ← Serialized config_settings_t
│ ┌─ upstream (36 bytes)              │
│ ├─ downstream (36 bytes)            │
│ ├─ usb_logging_enabled (1 byte)     │
│ ├─ protocol_logging_verbose (1 byte)│
│ └─ bill_table[8] (8 bytes)          │
├─────────────────────────────────────┤
│ crc16 (2 bytes)                     │ ← Checksum
│ padding (2 bytes)    = 0            │ ← Alignment
└─────────────────────────────────────┘
```


## PCB issues
- Tx3 works, but is not amplified by first Q due to assumed push-pull output of UART3. Switched to open drain in 101 onewiretest. Testing on 100 now

## UART Receive
- the receive function will be called by app.c with the interface object as argument
- receive will work based on sync, length and byte timing (5ms for CCNET)
- the interface object will have info on protocol, but let's maybe abstract that away
- we replace sync byte with header byte(s) as used in message.c
- the only thing uart.c needs to know is that the length field is the first after the header
- it will just create the raw message based on that length and return this as a message object. No need for uart to understand opcodes, data, crc

## Interrupt Call Hierarchies
- **Button**: `HAL_GPIO_EXTI_Callback()` (main.c) → `BTN_ConfigResetButtonInterrupt()` (btn.c) → `BTN_ProcessConfigResetButton()` (main loop)
- **Timer**: `HAL_TIM_PeriodElapsedCallback()` (tim.c) → registered callback function (one-shot)
- **UART**: `HAL_UART_RxCpltCallback()` (main.c) → `UART_RxCpltCallback()` (uart.c) → Clears flag, adds byte to ring buffer, sets rx_flag, restarts HAL_UART_Receive_IT → `UART_ProcessInterruptFlags()` (main loop) → `UART_ByteReceived()` (uart.c)


## Git

Next up: "feature: uart receive test". The comment for that (when finished) should be someting like:
- add test to receive 02 03 06 33 DA 81 on uart1
- log_proto the received bytes
- verify it parses correctly as raw data in message

### feature: parse uart received message
- uart.c receives data in parallel on two interfaces. If the data length byte is correct and timing constraints are met it is stored in messate_t.raw. Not more is done to it so far
- add receiving uart data in app.c main loop
- flag when .raw data is received
- parse .raw into a message. Check CRC, extract opcode and data
- add ascii of opcode to message for logging/debugging. See proto.h. Every protocol has its own opcodes. Also, make distinction between transmitting and receiving opcodes
- return state of the message in main loop (logical states: msg_ok, msg_unknown_opcode, msg_data_missing_for_opcode, msg_crc_wrong)
- log_warn if not msg_ok
- log_proto of opcode and raw message bytes in one

notes:
- crc validate function verified

# Tests
### Receive CCNET message at uart1:
Test Validation Points
✅ Protocol Detection: Correctly identifies CCNET (0x02 0x03) headers
✅ Direction Detection: Properly determines TX vs RX based on opcode
✅ Opcode Parsing: Validates known CCNET opcodes
✅ ASCII Conversion: Shows human-readable opcode names
✅ CRC Validation: Uses existing CRC validation system
✅ Error Handling: Comprehensive error reporting for all failure modes
✅ Statistics Tracking: Real-time success/failure monitoring
