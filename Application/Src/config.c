/**
  ******************************************************************************
  * @file           : config.c
  * @brief          : Configuration module implementation
  *                   Configuration management and CLI interface
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "usb.h"
#include "log.h"
#include "led.h"
#include "nvm.h"

/* External LED handle */
extern LED_HandleTypeDef hled3;

/* Private variables ---------------------------------------------------------*/

/* Global configuration settings */
config_settings_t g_config;

/* USB transmit buffer */
#define USB_TX_BUFFER_SIZE 1000
static char usb_tx_buffer[USB_TX_BUFFER_SIZE];
static uint16_t usb_tx_buffer_pos = 0;


/* Private function prototypes -----------------------------------------------*/
void CONFIG_DisplayBaudrateOptions(void);
void CONFIG_DisplayParityOptions(void);
void CONFIG_DisplayProtocolOptions(void);
void CONFIG_DisplayBillTableBinary(void);
static void CONFIG_DisplaySeparator(void);
static void CONFIG_BufferFlush(void);
static void CONFIG_BufferCheck(void);
static uint8_t CONFIG_WaitForInput(void);
static uint8_t CONFIG_ParseChoice(const char* input, uint8_t min, uint8_t max);
static void CONFIG_ExitMenu(void);
static nvm_result_t CONFIG_SerializeToBuffer(uint8_t* buffer, uint32_t* buffer_size);
static nvm_result_t CONFIG_DeserializeFromBuffer(const uint8_t* buffer, uint32_t buffer_size);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize configuration module
  * @retval None
  */
void CONFIG_Init(void)
{
    /* Initialize with default values */
    g_config.upstream.protocol = PROTO_CCNET;
    g_config.upstream.role = ROLE_BILL_VALIDATOR;
    g_config.upstream.phy.baudrate = 9600;
    g_config.upstream.phy.parity = UART_PARITY_NONE;
    g_config.upstream.phy.uart_polarity = POLARITY_NORMAL;
    g_config.upstream.datalink.polling_period_ms = 0; /* N/A for upstream */
    g_config.upstream.phy.uart_handle = &huart1;
    
    g_config.downstream.protocol = PROTO_ID003;
    g_config.downstream.role = ROLE_CONTROLLER;
    g_config.downstream.phy.baudrate = 9600;
    g_config.downstream.phy.parity = UART_PARITY_EVEN;
    g_config.downstream.phy.uart_polarity = POLARITY_NORMAL;
    g_config.downstream.datalink.polling_period_ms = 100;
    g_config.downstream.phy.uart_handle = &huart2;
    
    g_config.usb_logging_enabled = 1;
    g_config.protocol_logging_verbose = 0;
    
    /* Initialize bill table to all zeros */
    for (int i = 0; i < 8; i++)
    {
        g_config.bill_table[i] = 0;
    }
    
    /* Load settings from NVM - DISABLED FOR TESTING */
    CONFIG_LoadFromNVM();
    
}

/**
  * @brief  Load configuration from non-volatile memory
  * @retval None
  */
void CONFIG_LoadFromNVM(void)
{
    nvm_result_t result;
    uint8_t buffer[512];
    uint32_t buffer_size;
    
    /* Load serialized data from NVM */
    result = NVM_ReadConfigData(buffer, sizeof(buffer), &buffer_size);
    if (result == NVM_OK)
    {
        /* Deserialize configuration from buffer */
        result = CONFIG_DeserializeFromBuffer(buffer, buffer_size);
        if (result != NVM_OK)
        {
            LOG_Warn("Failed to deserialize configuration data, using defaults");
        }
    }
    else
    {
        LOG_Warn("Failed to load configuration from NVM (result: %d), using defaults", result);
    }
}

/**
  * @brief  Save configuration to non-volatile memory
  * @retval None
  */
void CONFIG_SaveToNVM(void)
{
    nvm_result_t result;
    uint8_t buffer[512];
    uint32_t buffer_size;
    
    /* Serialize configuration to buffer */
    result = CONFIG_SerializeToBuffer(buffer, &buffer_size);
    if (result == NVM_OK)
    {
        /* Save serialized data to NVM */
        result = NVM_WriteConfigData(buffer, buffer_size);
        if (result == NVM_OK)
        {
            CONFIG_BufferWrite("Configuration saved successfully!\r\n");
        }
        else
        {
            LOG_Error("Failed to save configuration to NVM: %d", result);
            CONFIG_BufferWrite("Error: Failed to save configuration!\r\n");
        }
    }
    else
    {
        LOG_Error("Failed to serialize configuration: %d", result);
        CONFIG_BufferWrite("Error: Failed to serialize configuration!\r\n");
    }
    
    // Flush buffer when save is complete
    CONFIG_FlushBuffer();
}


/**
  * @brief  Show configuration menu
  * @retval None
  */
void CONFIG_ShowMenu(void)
{
    CONFIG_BufferWrite("\r\n=== CONFIGURATION MENU ===\r\n");
    CONFIG_BufferWrite("1.  Upstream Protocol        (Current: CCNET - fixed)\r\n");
    
    CONFIG_BufferWrite("2.  Upstream Baudrate        (Current: ");
    CONFIG_DisplayBaudrate(g_config.upstream.phy.baudrate);
    CONFIG_BufferWrite(")\r\n");
    
    CONFIG_BufferWrite("3.  Upstream Parity          (Current: ");
    CONFIG_DisplayParity(g_config.upstream.phy.parity);
    CONFIG_BufferWrite(")\r\n");
    
    CONFIG_BufferWrite("4.  Downstream Protocol      (Current: ");
    CONFIG_DisplayProtocol(g_config.downstream.protocol);
    CONFIG_BufferWrite(")\r\n");
    
    CONFIG_BufferWrite("5.  Downstream Baudrate      (Current: ");
    CONFIG_DisplayBaudrate(g_config.downstream.phy.baudrate);
    CONFIG_BufferWrite(")\r\n");
    
    CONFIG_BufferWrite("6.  Downstream Parity        (Current: ");
    CONFIG_DisplayParity(g_config.downstream.phy.parity);
    CONFIG_BufferWrite(")\r\n");
    
    CONFIG_BufferWrite("7.  Downstream Polling       (Current: ");
    CONFIG_BufferWrite(g_config.downstream.datalink.polling_period_ms == 100 ? "100ms" : 
                       g_config.downstream.datalink.polling_period_ms == 200 ? "200ms" : 
                       g_config.downstream.datalink.polling_period_ms == 500 ? "500ms" : "1000ms");
    CONFIG_BufferWrite(")\r\n");
    
    CONFIG_BufferWrite("8.  Bill Table               (Current: Binary)\r\n");
    
    CONFIG_BufferWrite("9.  USB Logging              (Current: ");
    CONFIG_BufferWrite(g_config.usb_logging_enabled ? "Enabled" : "Disabled");
    CONFIG_BufferWrite(")\r\n");
    
    CONFIG_BufferWrite("10. Protocol Logging         (Current: ");
    CONFIG_BufferWrite(g_config.protocol_logging_verbose ? "Verbose" : "Short");
    CONFIG_BufferWrite(")\r\n");
    
    CONFIG_BufferWrite("11. Exit and Restart\r\n");
    CONFIG_BufferWrite("12. Save, Exit and Restart\r\n");
    CONFIG_BufferWrite("==========================\r\n");
    CONFIG_BufferWrite("Enter choice (1-12): ");
    
    // Flush buffer when menu is complete
    CONFIG_FlushBuffer();
}

/**
  * @brief  Process configuration menu
  * @retval None
  */
void CONFIG_ProcessMenu(void)
{
    // Check if input is ready
    if (USB_IsInputReady())
    {
        // Debug: Flash LED3 when input is ready (main loop context)
        LED_Flash(&hled3, 50);   /* 50 ms single flash */
        
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            // Parse the choice
            uint8_t choice = CONFIG_ParseChoice(input_buffer, 1, 12);
            
            // Process the choice
            if (choice > 0)
            {
                switch (choice)
                {
                    case CONFIG_MENU_UPSTREAM_PROTOCOL:
                        CONFIG_UpdateUpstreamProtocol();
                        break;
                    case CONFIG_MENU_UPSTREAM_BAUDRATE:
                        CONFIG_UpdateUpstreamBaudrate();
                        break;
                    case CONFIG_MENU_UPSTREAM_PARITY:
                        CONFIG_UpdateUpstreamParity();
                        break;
                    case CONFIG_MENU_DOWNSTREAM_PROTOCOL:
                        CONFIG_UpdateDownstreamProtocol();
                        break;
                    case CONFIG_MENU_DOWNSTREAM_BAUDRATE:
                        CONFIG_UpdateDownstreamBaudrate();
                        break;
                    case CONFIG_MENU_DOWNSTREAM_PARITY:
                        CONFIG_UpdateDownstreamParity();
                        break;
                    case CONFIG_MENU_DOWNSTREAM_POLLING:
                        CONFIG_UpdateDownstreamPolling();
                        break;
                    case CONFIG_MENU_BILL_TABLE:
                        CONFIG_UpdateBillTable();
                        break;
                    case CONFIG_MENU_USB_LOGGING:
                        CONFIG_UpdateUsbLogging();
                        break;
                    case CONFIG_MENU_PROTOCOL_LOGGING:
                        CONFIG_UpdateProtocolLogging();
                        break;
                    case CONFIG_MENU_EXIT:
                        CONFIG_ExitMenu();
                        return; // Exit immediately, don't show menu again
                    case 12: // Save, Exit and Restart
                        CONFIG_SaveToNVM();
                        CONFIG_ExitMenu();
                        return; // Exit immediately, don't show menu again
                    default:
                        CONFIG_BufferWrite("Invalid choice!\r\n");
                        CONFIG_FlushBuffer();
                        break;
                }
                
                // Show menu again after processing (except for exit)
                CONFIG_ShowMenu();
            }
            else
            {
                CONFIG_BufferWrite("Invalid choice! Please enter a number between 1 and 12: ");
                CONFIG_FlushBuffer();
            }
        }
    }
}

/**
  * @brief  Display current configuration settings
  * @retval None
  */
void CONFIG_DisplayCurrentSettings(void)
{
    USB_TransmitString("\r\n=== CURRENT CONFIGURATION ===\r\n");
    
    CONFIG_DisplayInterfaceSettings("Upstream", &g_config.upstream);
    CONFIG_DisplayInterfaceSettings("Downstream", &g_config.downstream);
    
    USB_TransmitString("Bill Table: ");
    CONFIG_DisplayBillTableBinary();
    USB_TransmitString("\r\n");
    
    USB_TransmitString("USB Logging: ");
    USB_TransmitString(g_config.usb_logging_enabled ? "Enabled" : "Disabled");
    USB_TransmitString("\r\n");
    
    USB_TransmitString("Protocol Logging: ");
    USB_TransmitString(g_config.protocol_logging_verbose ? "Verbose" : "Short");
    USB_TransmitString("\r\n");
    
    USB_TransmitString("==============================\r\n\r\n");
}

/**
  * @brief  Display interface settings
  * @param  name: interface name
  * @param  interface: interface configuration pointer
  * @retval None
  */
void CONFIG_DisplayInterfaceSettings(const char* name, interface_config_t* interface)
{
    USB_TransmitString(name);
    USB_TransmitString(" Interface:\r\n");
    
    /* Protocol */
    USB_TransmitString("  Protocol: ");
    switch (interface->protocol)
    {
        case PROTO_CCNET:
            USB_TransmitString("CCNET");
            break;
        case PROTO_ID003:
            USB_TransmitString("ID003");
            break;
        case PROTO_CCTALK:
            USB_TransmitString("CCTALK");
            break;
        default:
            USB_TransmitString("Unknown");
            break;
    }
    USB_TransmitString("\r\n");
    
    /* Baudrate */
    USB_TransmitString("  Baudrate: ");
    if (interface->phy.baudrate == 9600)
        USB_TransmitString("9600");
    else if (interface->phy.baudrate == 19200)
        USB_TransmitString("19200");
    else if (interface->phy.baudrate == 38400)
        USB_TransmitString("38400");
    else if (interface->phy.baudrate == 57600)
        USB_TransmitString("57600");
    else if (interface->phy.baudrate == 115200)
        USB_TransmitString("115200");
    else
        USB_TransmitString("Custom");
    USB_TransmitString("\r\n");
    
    /* Parity */
    USB_TransmitString("  Parity: ");
    if (interface->phy.parity == UART_PARITY_NONE)
        USB_TransmitString("None");
    else if (interface->phy.parity == UART_PARITY_EVEN)
        USB_TransmitString("Even");
    else if (interface->phy.parity == UART_PARITY_ODD)
        USB_TransmitString("Odd");
    else
        USB_TransmitString("Unknown");
    USB_TransmitString("\r\n");
    
    /* Polling period (only for downstream) */
    if (interface->datalink.polling_period_ms > 0)
    {
        USB_TransmitString("  Polling Period: ");
        if (interface->datalink.polling_period_ms == 50)
            USB_TransmitString("50ms");
        else if (interface->datalink.polling_period_ms == 100)
            USB_TransmitString("100ms");
        else if (interface->datalink.polling_period_ms == 200)
            USB_TransmitString("200ms");
        else if (interface->datalink.polling_period_ms == 500)
            USB_TransmitString("500ms");
        else
            USB_TransmitString("Custom");
        USB_TransmitString("\r\n");
    }
    else
    {
        USB_TransmitString("  Polling Period: N/A\r\n");
    }
    
    USB_TransmitString("\r\n");
}

/**
  * @brief  Update upstream protocol (fixed to CCNET)
  * @retval None
  */
void CONFIG_UpdateUpstreamProtocol(void)
{
    USB_TransmitString("Upstream protocol is fixed to CCNET and cannot be changed.\r\n");
}

/**
  * @brief  Update upstream baudrate
  * @retval None
  */
void CONFIG_UpdateUpstreamBaudrate(void)
{
    CONFIG_BufferWrite("\r\nSelect upstream baudrate:\r\n");
    CONFIG_DisplayBaudrateOptions();
    CONFIG_BufferWrite("\r\nEnter choice (1-5): ");
    CONFIG_FlushBuffer();
    
    // Wait for user input
    CONFIG_WaitForInput();
    char input_buffer[16];
    if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
    {
        uint8_t choice = CONFIG_ParseChoice(input_buffer, 1, 5);
        if (choice > 0)
        {
            switch (choice)
            {
                case 1: g_config.upstream.phy.baudrate = 9600; break;
                case 2: g_config.upstream.phy.baudrate = 19200; break;
                case 3: g_config.upstream.phy.baudrate = 38400; break;
                case 4: g_config.upstream.phy.baudrate = 57600; break;
                case 5: g_config.upstream.phy.baudrate = 115200; break;
            }
        }
        else
        {
            CONFIG_BufferWrite("Invalid choice! Using default (9600).\r\n");
            g_config.upstream.phy.baudrate = 9600;
        }
    }
    else
    {
        CONFIG_BufferWrite("No input received. Using default (9600).\r\n");
        g_config.upstream.phy.baudrate = 9600;
    }
    CONFIG_FlushBuffer();
}

/**
  * @brief  Update upstream parity
  * @retval None
  */
void CONFIG_UpdateUpstreamParity(void)
{
    CONFIG_BufferWrite("\r\nSelect upstream parity:\r\n");
    CONFIG_DisplayParityOptions();
    CONFIG_BufferWrite("\r\nEnter choice (1-3): ");
    CONFIG_FlushBuffer();
    
    // Wait for user input
    CONFIG_WaitForInput();
    char input_buffer[16];
    if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
    {
        uint8_t choice = CONFIG_ParseChoice(input_buffer, 1, 3);
        if (choice > 0)
        {
            switch (choice)
            {
                case 1: g_config.upstream.phy.parity = UART_PARITY_NONE; break;
                case 2: g_config.upstream.phy.parity = UART_PARITY_EVEN; break;
                case 3: g_config.upstream.phy.parity = UART_PARITY_ODD; break;
            }
        }
        else
        {
            CONFIG_BufferWrite("Invalid choice! Using default (None).\r\n");
            g_config.upstream.phy.parity = UART_PARITY_NONE;
        }
    }
    else
    {
        CONFIG_BufferWrite("No input received. Using default (None).\r\n");
        g_config.upstream.phy.parity = UART_PARITY_NONE;
    }
    CONFIG_FlushBuffer();
}

/**
  * @brief  Update downstream protocol
  * @retval None
  */
void CONFIG_UpdateDownstreamProtocol(void)
{
    CONFIG_BufferWrite("\r\nSelect downstream protocol:\r\n");
    CONFIG_DisplayProtocolOptions();
    CONFIG_BufferWrite("\r\nEnter choice (1-2): ");
    CONFIG_FlushBuffer();
    
    // Wait for user input
    CONFIG_WaitForInput();
    char input_buffer[16];
    if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
    {
        uint8_t choice = CONFIG_ParseChoice(input_buffer, 1, 2);
        if (choice > 0)
        {
            switch (choice)
            {
                case 1: g_config.downstream.protocol = PROTO_ID003; break;
                case 2: g_config.downstream.protocol = PROTO_CCTALK; break;
            }
        }
        else
        {
            CONFIG_BufferWrite("Invalid choice! Using default (ID003).\r\n");
            g_config.downstream.protocol = PROTO_ID003;
        }
    }
    else
    {
        CONFIG_BufferWrite("No input received. Using default (ID003).\r\n");
        g_config.downstream.protocol = PROTO_ID003;
    }
    CONFIG_FlushBuffer();
}

/**
  * @brief  Update downstream baudrate
  * @retval None
  */
void CONFIG_UpdateDownstreamBaudrate(void)
{
    CONFIG_BufferWrite("\r\nSelect downstream baudrate:\r\n");
    CONFIG_DisplayBaudrateOptions();
    CONFIG_BufferWrite("\r\nEnter choice (1-5): ");
    CONFIG_FlushBuffer();
    
    // Wait for user input
    if (CONFIG_WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t choice = CONFIG_ParseChoice(input_buffer, 1, 5);
            if (choice > 0)
            {
                switch (choice)
                {
                    case 1: g_config.downstream.phy.baudrate = 9600; break;
                    case 2: g_config.downstream.phy.baudrate = 19200; break;
                    case 3: g_config.downstream.phy.baudrate = 38400; break;
                    case 4: g_config.downstream.phy.baudrate = 57600; break;
                    case 5: g_config.downstream.phy.baudrate = 115200; break;
                }
            }
            else
            {
                CONFIG_BufferWrite("Invalid choice! Using default (9600).\r\n");
                g_config.downstream.phy.baudrate = 9600;
            }
        }
        else
        {
            CONFIG_BufferWrite("No input received. Using default (9600).\r\n");
            g_config.downstream.phy.baudrate = 9600;
        }
    }
    else
    {
        CONFIG_BufferWrite("No input received. Using default (9600).\r\n");
        g_config.downstream.phy.baudrate = 9600;
    }
    CONFIG_FlushBuffer();
}

/**
  * @brief  Update downstream parity
  * @retval None
  */
void CONFIG_UpdateDownstreamParity(void)
{
    CONFIG_BufferWrite("\r\nSelect downstream parity:\r\n");
    CONFIG_DisplayParityOptions();
    CONFIG_BufferWrite("\r\nEnter choice (1-3): ");
    CONFIG_FlushBuffer();
    
    // Wait for user input
    if (CONFIG_WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t choice = CONFIG_ParseChoice(input_buffer, 1, 3);
            if (choice > 0)
            {
                switch (choice)
                {
                    case 1: g_config.downstream.phy.parity = UART_PARITY_NONE; break;
                    case 2: g_config.downstream.phy.parity = UART_PARITY_EVEN; break;
                    case 3: g_config.downstream.phy.parity = UART_PARITY_ODD; break;
                }
            }
            else
            {
                CONFIG_BufferWrite("Invalid choice! Using default (Even).\r\n");
                g_config.downstream.phy.parity = UART_PARITY_EVEN;
            }
        }
        else
        {
            CONFIG_BufferWrite("No input received. Using default (Even).\r\n");
            g_config.downstream.phy.parity = UART_PARITY_EVEN;
        }
    }
    else
    {
        CONFIG_BufferWrite("No input received. Using default (Even).\r\n");
        g_config.downstream.phy.parity = UART_PARITY_EVEN;
    }
    CONFIG_FlushBuffer();
}

/**
  * @brief  Update downstream polling period
  * @retval None
  */
void CONFIG_UpdateDownstreamPolling(void)
{
    CONFIG_BufferWrite("\r\nSelect downstream polling period:\r\n");
    CONFIG_BufferWrite("1. 100ms\r\n");
    CONFIG_BufferWrite("2. 200ms\r\n");
    CONFIG_BufferWrite("3. 500ms\r\n");
    CONFIG_BufferWrite("4. 1000ms\r\n");
    CONFIG_BufferWrite("\r\nEnter choice (1-4): ");
    CONFIG_FlushBuffer();
    
    // Wait for user input
    if (CONFIG_WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t choice = CONFIG_ParseChoice(input_buffer, 1, 4);
            if (choice > 0)
            {
                switch (choice)
                {
                    case 1: g_config.downstream.datalink.polling_period_ms = 100; break;
                    case 2: g_config.downstream.datalink.polling_period_ms = 200; break;
                    case 3: g_config.downstream.datalink.polling_period_ms = 500; break;
                    case 4: g_config.downstream.datalink.polling_period_ms = 1000; break;
                }
            }
            else
            {
                CONFIG_BufferWrite("Invalid choice! Using default (100ms).\r\n");
                g_config.downstream.datalink.polling_period_ms = 100;
            }
        }
        else
        {
            CONFIG_BufferWrite("No input received. Using default (100ms).\r\n");
            g_config.downstream.datalink.polling_period_ms = 100;
        }
    }
    else
    {
        CONFIG_BufferWrite("No input received. Using default (100ms).\r\n");
        g_config.downstream.datalink.polling_period_ms = 100;
    }
    CONFIG_FlushBuffer();
}

/**
  * @brief  Update bill table
  * @retval None
  */
void CONFIG_UpdateBillTable(void)
{
    CONFIG_BufferWrite("\r\nCurrent bill table: ");
    CONFIG_DisplayBillTableBinary();
    CONFIG_BufferWrite("\r\n");
    CONFIG_BufferWrite("Enter new bill table (8 bits, e.g., 10101010): ");
    CONFIG_FlushBuffer();
    
    // Wait for user input
    if (CONFIG_WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            // Parse binary input
            uint8_t valid = 1;
            for (int i = 0; i < 8; i++)
            {
                if (input_buffer[i] == '1')
                {
                    g_config.bill_table[i] = 1;
                }
                else if (input_buffer[i] == '0')
                {
                    g_config.bill_table[i] = 0;
                }
                else
                {
                    valid = 0;
                    break;
                }
            }
            
            if (valid)
            {
                CONFIG_BufferWrite("Bill table updated successfully.\r\n");
            }
            else
            {
                CONFIG_BufferWrite("Invalid input! Bill table unchanged.\r\n");
            }
        }
        else
        {
            CONFIG_BufferWrite("No input received. Bill table unchanged.\r\n");
        }
    }
    else
    {
        CONFIG_BufferWrite("Timeout. Bill table unchanged.\r\n");
    }
    CONFIG_FlushBuffer();
}

/**
  * @brief  Update USB logging setting
  * @retval None
  */
void CONFIG_UpdateUsbLogging(void)
{
    CONFIG_BufferWrite("\r\nUSB Logging: ");
    CONFIG_BufferWrite(g_config.usb_logging_enabled ? "Enabled" : "Disabled");
    CONFIG_BufferWrite("\r\n");
    CONFIG_BufferWrite("1. Enable\r\n");
    CONFIG_BufferWrite("2. Disable\r\n");
    CONFIG_BufferWrite("\r\nEnter choice (1-2): ");
    CONFIG_FlushBuffer();
    
    // Wait for user input
    if (CONFIG_WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t choice = CONFIG_ParseChoice(input_buffer, 1, 2);
            if (choice > 0)
            {
                g_config.usb_logging_enabled = (choice == 1) ? 1 : 0;
            }
            else
            {
                CONFIG_BufferWrite("Invalid choice! Using default (Disabled).\r\n");
                g_config.usb_logging_enabled = 0;
            }
        }
        else
        {
            CONFIG_BufferWrite("No input received. Using default (Disabled).\r\n");
            g_config.usb_logging_enabled = 0;
        }
    }
    else
    {
        CONFIG_BufferWrite("No input received. Using default (Disabled).\r\n");
        g_config.usb_logging_enabled = 0;
    }
    CONFIG_FlushBuffer();
}

/**
  * @brief  Update protocol logging setting
  * @retval None
  */
void CONFIG_UpdateProtocolLogging(void)
{
    CONFIG_BufferWrite("\r\nProtocol Logging: ");
    CONFIG_BufferWrite(g_config.protocol_logging_verbose ? "Verbose" : "Short");
    CONFIG_BufferWrite("\r\n");
    CONFIG_BufferWrite("1. Short\r\n");
    CONFIG_BufferWrite("2. Verbose\r\n");
    CONFIG_BufferWrite("\r\nEnter choice (1-2): ");
    CONFIG_FlushBuffer();
    
    // Wait for user input
    if (CONFIG_WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t choice = CONFIG_ParseChoice(input_buffer, 1, 2);
            if (choice > 0)
            {
                g_config.protocol_logging_verbose = (choice == 2) ? 1 : 0;
            }
            else
            {
                CONFIG_BufferWrite("Invalid choice! Using default (Disabled).\r\n");
                g_config.protocol_logging_verbose = 0;
            }
        }
        else
        {
            CONFIG_BufferWrite("No input received. Using default (Short).\r\n");
            g_config.protocol_logging_verbose = 0;
        }
    }
    else
    {
        CONFIG_BufferWrite("No input received. Using default (Short).\r\n");
        g_config.protocol_logging_verbose = 0;
    }
    CONFIG_FlushBuffer();
}

/**
  * @brief  Flush USB transmit buffer (public function)
  * @retval None
  */
void CONFIG_FlushBuffer(void)
{
    CONFIG_BufferFlush();
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Display baudrate options
  * @retval None
  */
void CONFIG_DisplayBaudrateOptions(void)
{
    CONFIG_BufferWrite("1. 9600\r\n");
    CONFIG_BufferWrite("2. 19200\r\n");
    CONFIG_BufferWrite("3. 38400\r\n");
    CONFIG_BufferWrite("4. 57600\r\n");
    CONFIG_BufferWrite("5. 115200\r\n");
}

/**
  * @brief  Display parity options
  * @retval None
  */
void CONFIG_DisplayParityOptions(void)
{
    CONFIG_BufferWrite("1. None\r\n");
    CONFIG_BufferWrite("2. Even\r\n");
    CONFIG_BufferWrite("3. Odd\r\n");
}

/**
  * @brief  Display protocol options
  * @retval None
  */
void CONFIG_DisplayProtocolOptions(void)
{
    CONFIG_BufferWrite("1. ID003\r\n");
    CONFIG_BufferWrite("2. CCTalk\r\n");
}

/**
  * @brief  Display bill table in binary format
  * @retval None
  */
void CONFIG_DisplayBillTableBinary(void)
{
    for (int i = 0; i < 8; i++)
    {
        CONFIG_BufferWrite(g_config.bill_table[i] ? "1" : "0");
    }
    CONFIG_BufferWrite("\r\n");
}


/**
  * @brief  Display separator line
  * @retval None
  */
static void CONFIG_DisplaySeparator(void)
{
    CONFIG_BufferWrite("--------------------------------\r\n");
}

/**
  * @brief  Write string to USB transmit buffer
  * @param  str: string to write to buffer
  * @retval None
  */
void CONFIG_BufferWrite(const char* str)
{
    if (str == NULL) return;
    
    // Write string to buffer
    while (*str != '\0' && usb_tx_buffer_pos < USB_TX_BUFFER_SIZE) {
        usb_tx_buffer[usb_tx_buffer_pos++] = *str++;
    }
    
    // Check if buffer should be flushed
    CONFIG_BufferCheck();
}

/**
  * @brief  Flush USB transmit buffer
  * @retval None
  */
static void CONFIG_BufferFlush(void)
{
    if (usb_tx_buffer_pos > 0) {
        // Send buffer content
        USB_TransmitBytes((uint8_t*)usb_tx_buffer, usb_tx_buffer_pos);
        
        // Reset buffer position
        usb_tx_buffer_pos = 0;
    }
}

/**
  * @brief  Check if buffer should be flushed
  * @retval None
  */
static void CONFIG_BufferCheck(void)
{
    // Flush if buffer is > 900 bytes or buffer is full
    if (usb_tx_buffer_pos > 900 || usb_tx_buffer_pos >= USB_TX_BUFFER_SIZE) {
        CONFIG_BufferFlush();
    }
}

/**
  * @brief  Display protocol name
  * @param  protocol: protocol type
  * @retval None
  */
void CONFIG_DisplayProtocol(uint8_t protocol)
{
    switch (protocol)
    {
        case PROTO_CCNET:
            CONFIG_BufferWrite("CCNET");
            break;
        case PROTO_ID003:
            CONFIG_BufferWrite("ID003");
            break;
        case PROTO_CCTALK:
            CONFIG_BufferWrite("CCTalk");
            break;
        default:
            CONFIG_BufferWrite("Unknown");
            break;
    }
}

/**
  * @brief  Display baudrate value
  * @param  baudrate: baudrate value
  * @retval None
  */
void CONFIG_DisplayBaudrate(uint32_t baudrate)
{
    if (baudrate == 9600) CONFIG_BufferWrite("9600");
    else if (baudrate == 19200) CONFIG_BufferWrite("19200");
    else if (baudrate == 38400) CONFIG_BufferWrite("38400");
    else if (baudrate == 57600) CONFIG_BufferWrite("57600");
    else if (baudrate == 115200) CONFIG_BufferWrite("115200");
    else CONFIG_BufferWrite("9600");
}

/**
  * @brief  Display parity name
  * @param  parity: parity type
  * @retval None
  */
void CONFIG_DisplayParity(uint32_t parity)
{
    switch (parity)
    {
        case UART_PARITY_NONE:
            CONFIG_BufferWrite("None");
            break;
        case UART_PARITY_EVEN:
            CONFIG_BufferWrite("Even");
            break;
        case UART_PARITY_ODD:
            CONFIG_BufferWrite("Odd");
            break;
        default:
            CONFIG_BufferWrite("Unknown");
            break;
    }
}

/**
  * @brief  Wait for user input (no timeout)
  * @retval 1 when input received
  */
static uint8_t CONFIG_WaitForInput(void)
{
    while (1)
    {
        if (USB_IsInputReady())
        {
            return 1;
        }
        HAL_Delay(10); // Small delay to prevent busy waiting
    }
}

/**
  * @brief  Parse user choice from input string
  * @param  input: input string
  * @param  min: minimum valid choice
  * @param  max: maximum valid choice
  * @retval Parsed choice value, or 0 if invalid
  */
static uint8_t CONFIG_ParseChoice(const char* input, uint8_t min, uint8_t max)
{
    if (input == NULL) return 0;
    
    // Skip whitespace
    while (*input == ' ' || *input == '\t' || *input == '\r' || *input == '\n')
    {
        input++;
    }
    
    // Check if input is empty
    if (*input == '\0') return 0;
    
    // Parse number
    uint8_t choice = 0;
    while (*input >= '0' && *input <= '9')
    {
        choice = choice * 10 + (*input - '0');
        input++;
    }
    
    // Check if choice is within valid range
    if (choice >= min && choice <= max)
    {
        return choice;
    }
    
    return 0; // Invalid choice
}

/**
  * @brief  Exit configuration menu
  * @retval None
  */
static void CONFIG_ExitMenu(void)
{
    CONFIG_BufferWrite("Exiting configuration menu...\r\n");
    CONFIG_BufferWrite("Restarting MCU...\r\n");
    CONFIG_FlushBuffer();
    
    // Restart the MCU
    HAL_Delay(100);  // Give time for USB transmission to complete
    HAL_NVIC_SystemReset();
}

/**
  * @brief  Serialize configuration to buffer
  * @param  buffer: Pointer to buffer to store serialized data
  * @param  buffer_size: Pointer to receive actual buffer size used
  * @retval NVM operation result
  */
static nvm_result_t CONFIG_SerializeToBuffer(uint8_t* buffer, uint32_t* buffer_size)
{
    if (buffer == NULL || buffer_size == NULL)
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Simple binary serialization - direct memory copy */
    uint32_t config_size = sizeof(config_settings_t);
    
    if (config_size > 512)
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Copy configuration structure to buffer */
    for (uint32_t i = 0; i < config_size; i++)
    {
        buffer[i] = ((uint8_t*)&g_config)[i];
    }
    
    *buffer_size = config_size;
    return NVM_OK;
}

/**
  * @brief  Deserialize configuration from buffer
  * @param  buffer: Pointer to buffer containing serialized data
  * @param  buffer_size: Size of buffer data
  * @retval NVM operation result
  */
static nvm_result_t CONFIG_DeserializeFromBuffer(const uint8_t* buffer, uint32_t buffer_size)
{
    if (buffer == NULL)
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Check if buffer size matches expected configuration size */
    uint32_t expected_size = sizeof(config_settings_t);
    
    if (buffer_size != expected_size)
    {
        LOG_Error("Buffer size %lu does not match config size %lu", buffer_size, expected_size);
        return NVM_INVALID_PARAM;
    }
    
    /* Copy buffer data to configuration structure */
    for (uint32_t i = 0; i < expected_size; i++)
    {
        ((uint8_t*)&g_config)[i] = buffer[i];
    }
    
    return NVM_OK;
}
