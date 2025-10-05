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



/* Private function prototypes -----------------------------------------------*/
static void CONFIG_MemCpy(uint8_t* dest, const uint8_t* src, uint32_t length);
static void CONFIG_SetPhy(interface_config_t* interface);
static void CONFIG_SetDataLink(interface_config_t* interface);
static void CONFIG_DisplayBaudrateOptions(void);
static void CONFIG_DisplayParityOptions(void);
static void CONFIG_DisplayProtocolOptions(void);
static void CONFIG_DisplayBillTableBinary(void);
static void CONFIG_DisplayProtocol(uint8_t protocol);
static void CONFIG_DisplayBaudrate(uint32_t baudrate);
static void CONFIG_DisplayParity(uint32_t parity);
static void CONFIG_DisplayInterfaceSettings(const char* name, interface_config_t* interface);
static void CONFIG_ShowConfiguration(void);
static void CONFIG_UpdateUpstreamProtocol(void);
static void CONFIG_UpdateUpstreamBaudrate(void);
static void CONFIG_UpdateUpstreamParity(void);
static void CONFIG_UpdateDownstreamProtocol(void);
static void CONFIG_UpdateDownstreamBaudrate(void);
static void CONFIG_UpdateDownstreamParity(void);
static void CONFIG_UpdateDownstreamPolling(void);
static void CONFIG_UpdateBillTable(void);
static void CONFIG_UpdateUsbLogging(void);
static void CONFIG_UpdateProtocolLogging(void);
static void CONFIG_DisplaySeparator(void);
static void CONFIG_DisplayEnterChoice(uint8_t max_choice);
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
    /* Point to default UART interface objects set in app.c as fallbacks */
    /* These provide default values if reading from flash fails */
    g_config.upstream = &if_upstream;
    g_config.downstream = &if_downstream;
    
    g_config.usb_logging_enabled = 1;
    g_config.protocol_logging_verbose = 0;
    
    /* Initialize bill table to all zeros */
    for (int i = 0; i < 8; i++)
    {
        g_config.bill_table[i] = 0;
    }
    
    /* Load settings from NVM and store in if_upstream and if_downstream */
    CONFIG_LoadFromNVM();
    CONFIG_SetPhy(&if_upstream);
    CONFIG_SetPhy(&if_downstream);
    CONFIG_SetDataLink(&if_upstream);
    CONFIG_SetDataLink(&if_downstream);
    
    /* Display current settings */
    CONFIG_ShowConfiguration();
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
        else {
            LOG_Info("Configuration loaded from flash successfully");
        }
    }
    else
    {
        LOG_Warn("Failed to load configuration from NVM, using defaults");
    }
}

/**
  * @brief  Set protocol-specific phy (uart) configuration for interface
  * @param  interface: Pointer to interface configuration structure
  * @note   Sets sync bytes, length offset, CRC length, and timeout based on protocol.
  *         Polling period is preserved from flash storage.
  * @retval None
  */
static void CONFIG_SetPhy(interface_config_t* interface)
{
    switch (interface->protocol)
    {
        case PROTO_CCNET:
            interface->phy.uart_handle = &huart1;
            break;
        case PROTO_ID003:
            interface->phy.uart_handle = &huart2;
            break;
        case PROTO_CCTALK:
            interface->phy.uart_handle = &huart3;
            break;
        default:
            LOG_Error("Unknown protocol");
            break;
    }
}

/**
  * @brief  Set protocol-specific datalink configuration for interface
  * @param  interface: Pointer to interface configuration structure
  * @note   Sets sync bytes, length offset, CRC length, and timeout based on protocol.
  *         Polling period is preserved from flash storage.
  * @retval None
  */
  static void CONFIG_SetDataLink(interface_config_t* interface)
  {
      /* Set data link configuration in interface object based on protocol read from flash */
      /* only polling period (relevant for downstream) is stored in flash */
      switch (interface->protocol)
      {
          case PROTO_CCNET:
              interface->datalink.sync_length = 2;
              interface->datalink.sync_byte1 = 0x02;
              interface->datalink.sync_byte2 = 0x03;
              interface->datalink.length_offset = 0;
              interface->datalink.crc_length = 2;
              interface->datalink.inter_byte_timeout_ms = 5;
              break;
          case PROTO_ID003:
              interface->datalink.sync_length = 1;
              interface->datalink.sync_byte1 = 0xFC;
              interface->datalink.sync_byte2 = 0x00;
              interface->datalink.length_offset = 0;
              interface->datalink.crc_length = 2;
              interface->datalink.inter_byte_timeout_ms = 5;
              break;
          case PROTO_CCTALK:
              interface->datalink.sync_length = 0;
              interface->datalink.sync_byte1 = 0x00;
              interface->datalink.sync_byte2 = 0x00;
              interface->datalink.length_offset = -5;
              interface->datalink.crc_length = 1;
              interface->datalink.inter_byte_timeout_ms = 5;
              break;
          default:
              LOG_Error("Unknown protocol");
              break;
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
            USB_TransmitString("Configuration saved successfully!\r\n");
        }
        else
        {
            LOG_Error("Failed to save configuration to NVM");
            USB_TransmitString("Error: Failed to save configuration!\r\n");
        }
    }
    else
    {
        LOG_Error("Failed to serialize configuration");
        USB_TransmitString("Error: Failed to serialize configuration!\r\n");
    }
}

void CONFIG_ShowMenu(void)
{
    CONFIG_ShowConfiguration();
    HAL_Delay(100); /* 100ms delay to ensure the configuration is displayed */
    
    USB_TransmitString("11. Exit and Restart\r\n");
    USB_TransmitString("12. Save, Exit and Restart\r\n");
    USB_TransmitString("==========================\r\n");
    CONFIG_DisplayEnterChoice(12);
    HAL_Delay(100);
    USB_Flush();
}

/**
  * @brief  Show configuration menu
  * @retval None
  */
static void CONFIG_ShowConfiguration(void)
{
    USB_TransmitString("\r\n=== CONFIGURATION ===\r\n");
    USB_TransmitString("1.  Upstream Protocol        : CCNET - fixed\r\n");
    
    USB_TransmitString("2.  Upstream Baudrate        : ");
    CONFIG_DisplayBaudrate(g_config.upstream->phy.baudrate);
    USB_TransmitString("\r\n");
    
    USB_TransmitString("3.  Upstream Parity          : ");
    CONFIG_DisplayParity(g_config.upstream->phy.parity);
    USB_TransmitString("\r\n");
    
    USB_TransmitString("4.  Downstream Protocol      : ");
    CONFIG_DisplayProtocol(g_config.downstream->protocol);
    USB_TransmitString("\r\n");
    
    USB_TransmitString("5.  Downstream Baudrate      : ");
    CONFIG_DisplayBaudrate(g_config.downstream->phy.baudrate);
    USB_TransmitString("\r\n");
    
    USB_TransmitString("6.  Downstream Parity        : ");
    CONFIG_DisplayParity(g_config.downstream->phy.parity);
    USB_TransmitString("\r\n");
    
    USB_TransmitString("7.  Downstream Polling       : ");
    USB_TransmitString(g_config.downstream->datalink.polling_period_ms == 100 ? "100ms" : 
                       g_config.downstream->datalink.polling_period_ms == 200 ? "200ms" : 
                       g_config.downstream->datalink.polling_period_ms == 500 ? "500ms" : "1000ms");
    USB_TransmitString("\r\n");
    
    USB_TransmitString("8.  Bill Table               : Binary\r\n");
    
    USB_TransmitString("9.  USB Logging              : ");
    USB_TransmitString(g_config.usb_logging_enabled ? "Enabled" : "Disabled");
    USB_TransmitString("\r\n");
    
    USB_TransmitString("10. Protocol Logging         : ");
    USB_TransmitString(g_config.protocol_logging_verbose ? "Verbose" : "Short");
    USB_TransmitString("\r\n");
    USB_TransmitString("======================\r\n\r\n");
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
                        USB_TransmitString("Invalid choice!\r\n");
                        break;
                }
                
                // Show menu again after processing (except for exit)
                CONFIG_ShowMenu();
            }
            else
            {
                USB_TransmitString("Invalid choice! Please enter a number between 1 and 12: ");
            }
        }
    }
}


/**
  * @brief  Display interface settings
  * @param  name: interface name
  * @param  interface: interface configuration pointer
  * @retval None
  */
static void CONFIG_DisplayInterfaceSettings(const char* name, interface_config_t* interface)
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
static void CONFIG_UpdateUpstreamProtocol(void)
{
    USB_TransmitString("Upstream protocol is fixed to CCNET and cannot be changed.\r\n");
}

/**
  * @brief  Update upstream baudrate
  * @retval None
  */
static void CONFIG_UpdateUpstreamBaudrate(void)
{
    USB_TransmitString("\r\nSelect upstream baudrate:\r\n");
    CONFIG_DisplayBaudrateOptions();
    CONFIG_DisplayEnterChoice(5);
    
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
                case 1: g_config.upstream->phy.baudrate = 9600; break;
                case 2: g_config.upstream->phy.baudrate = 19200; break;
                case 3: g_config.upstream->phy.baudrate = 38400; break;
                case 4: g_config.upstream->phy.baudrate = 57600; break;
                case 5: g_config.upstream->phy.baudrate = 115200; break;
            }
        }
        else
        {
            USB_TransmitString("Invalid choice! Using default (9600).\r\n");
            g_config.upstream->phy.baudrate = 9600;
        }
    }
    else
    {
        USB_TransmitString("No input received. Using default (9600).\r\n");
        g_config.upstream->phy.baudrate = 9600;
    }
}

/**
  * @brief  Update upstream parity
  * @retval None
  */
static void CONFIG_UpdateUpstreamParity(void)
{
    USB_TransmitString("\r\nSelect upstream parity:\r\n");
    CONFIG_DisplayParityOptions();
    CONFIG_DisplayEnterChoice(3);
    
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
                case 1: g_config.upstream->phy.parity = UART_PARITY_NONE; break;
                case 2: g_config.upstream->phy.parity = UART_PARITY_EVEN; break;
                case 3: g_config.upstream->phy.parity = UART_PARITY_ODD; break;
            }
        }
        else
        {
            USB_TransmitString("Invalid choice! Using default (None).\r\n");
            g_config.upstream->phy.parity = UART_PARITY_NONE;
        }
    }
    else
    {
        USB_TransmitString("No input received. Using default (None).\r\n");
        g_config.upstream->phy.parity = UART_PARITY_NONE;
    }
}

/**
  * @brief  Update downstream protocol
  * @retval None
  */
static void CONFIG_UpdateDownstreamProtocol(void)
{
    USB_TransmitString("\r\nSelect downstream protocol:\r\n");
    CONFIG_DisplayProtocolOptions();
    CONFIG_DisplayEnterChoice(2);
    
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
                case 1: g_config.downstream->protocol = PROTO_ID003; break;
                case 2: g_config.downstream->protocol = PROTO_CCTALK; break;
            }
        }
        else
        {
            USB_TransmitString("Invalid choice! Using default (ID003).\r\n");
            g_config.downstream->protocol = PROTO_ID003;
        }
    }
    else
    {
        USB_TransmitString("No input received. Using default (ID003).\r\n");
        g_config.downstream->protocol = PROTO_ID003;
    }
}

/**
  * @brief  Update downstream baudrate
  * @retval None
  */
static void CONFIG_UpdateDownstreamBaudrate(void)
{
    USB_TransmitString("\r\nSelect downstream baudrate:\r\n");
    CONFIG_DisplayBaudrateOptions();
    CONFIG_DisplayEnterChoice(5);
    
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
                    case 1: g_config.downstream->phy.baudrate = 9600; break;
                    case 2: g_config.downstream->phy.baudrate = 19200; break;
                    case 3: g_config.downstream->phy.baudrate = 38400; break;
                    case 4: g_config.downstream->phy.baudrate = 57600; break;
                    case 5: g_config.downstream->phy.baudrate = 115200; break;
                }
            }
            else
            {
                USB_TransmitString("Invalid choice! Using default (9600).\r\n");
                g_config.downstream->phy.baudrate = 9600;
            }
        }
        else
        {
            USB_TransmitString("No input received. Using default (9600).\r\n");
            g_config.downstream->phy.baudrate = 9600;
        }
    }
    else
    {
        USB_TransmitString("No input received. Using default (9600).\r\n");
        g_config.downstream->phy.baudrate = 9600;
    }
}

/**
  * @brief  Update downstream parity
  * @retval None
  */
static void CONFIG_UpdateDownstreamParity(void)
{
    USB_TransmitString("\r\nSelect downstream parity:\r\n");
    CONFIG_DisplayParityOptions();
    CONFIG_DisplayEnterChoice(3);
    
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
                    case 1: g_config.downstream->phy.parity = UART_PARITY_NONE; break;
                    case 2: g_config.downstream->phy.parity = UART_PARITY_EVEN; break;
                    case 3: g_config.downstream->phy.parity = UART_PARITY_ODD; break;
                }
            }
            else
            {
                USB_TransmitString("Invalid choice! Using default (Even).\r\n");
                g_config.downstream->phy.parity = UART_PARITY_EVEN;
            }
        }
        else
        {
            USB_TransmitString("No input received. Using default (Even).\r\n");
            g_config.downstream->phy.parity = UART_PARITY_EVEN;
        }
    }
    else
    {
        USB_TransmitString("No input received. Using default (Even).\r\n");
        g_config.downstream->phy.parity = UART_PARITY_EVEN;
    }
}

/**
  * @brief  Update downstream polling period
  * @retval None
  */
static void CONFIG_UpdateDownstreamPolling(void)
{
    USB_TransmitString("\r\nSelect downstream polling period:\r\n");
    USB_TransmitString("1. 100ms\r\n");
    USB_TransmitString("2. 200ms\r\n");
    USB_TransmitString("3. 500ms\r\n");
    USB_TransmitString("4. 1000ms\r\n");
    CONFIG_DisplayEnterChoice(4);
    
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
                    case 1: g_config.downstream->datalink.polling_period_ms = 100; break;
                    case 2: g_config.downstream->datalink.polling_period_ms = 200; break;
                    case 3: g_config.downstream->datalink.polling_period_ms = 500; break;
                    case 4: g_config.downstream->datalink.polling_period_ms = 1000; break;
                }
            }
            else
            {
                USB_TransmitString("Invalid choice! Using default (100ms).\r\n");
                g_config.downstream->datalink.polling_period_ms = 100;
            }
        }
        else
        {
            USB_TransmitString("No input received. Using default (100ms).\r\n");
            g_config.downstream->datalink.polling_period_ms = 100;
        }
    }
    else
    {
        USB_TransmitString("No input received. Using default (100ms).\r\n");
        g_config.downstream->datalink.polling_period_ms = 100;
    }
}

/**
  * @brief  Update bill table
  * @retval None
  */
static void CONFIG_UpdateBillTable(void)
{
    USB_TransmitString("\r\nCurrent bill table: ");
    CONFIG_DisplayBillTableBinary();
    USB_TransmitString("\r\n");
    USB_TransmitString("Enter new bill table (8 bits, e.g., 10101010): ");
    
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
                USB_TransmitString("Bill table updated successfully.\r\n");
            }
            else
            {
                USB_TransmitString("Invalid input! Bill table unchanged.\r\n");
            }
        }
        else
        {
            USB_TransmitString("No input received. Bill table unchanged.\r\n");
        }
    }
    else
    {
        USB_TransmitString("Timeout. Bill table unchanged.\r\n");
    }
}

/**
  * @brief  Update USB logging setting
  * @retval None
  */
static void CONFIG_UpdateUsbLogging(void)
{
    USB_TransmitString("\r\nUSB Logging: ");
    USB_TransmitString(g_config.usb_logging_enabled ? "Enabled" : "Disabled");
    USB_TransmitString("\r\n");
    USB_TransmitString("1. Enable\r\n");
    USB_TransmitString("2. Disable\r\n");
    CONFIG_DisplayEnterChoice(2);
    
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
                USB_TransmitString("Invalid choice! Using default (Disabled).\r\n");
                g_config.usb_logging_enabled = 0;
            }
        }
        else
        {
            USB_TransmitString("No input received. Using default (Disabled).\r\n");
            g_config.usb_logging_enabled = 0;
        }
    }
    else
    {
        USB_TransmitString("No input received. Using default (Disabled).\r\n");
        g_config.usb_logging_enabled = 0;
    }
}

/**
  * @brief  Update protocol logging setting
  * @retval None
  */
static void CONFIG_UpdateProtocolLogging(void)
{
    USB_TransmitString("\r\nProtocol Logging: ");
    USB_TransmitString(g_config.protocol_logging_verbose ? "Verbose" : "Short");
    USB_TransmitString("\r\n");
    USB_TransmitString("1. Short\r\n");
    USB_TransmitString("2. Verbose\r\n");
    CONFIG_DisplayEnterChoice(2);
    
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
                USB_TransmitString("Invalid choice! Using default (Disabled).\r\n");
                g_config.protocol_logging_verbose = 0;
            }
        }
        else
        {
            USB_TransmitString("No input received. Using default (Short).\r\n");
            g_config.protocol_logging_verbose = 0;
        }
    }
    else
    {
        USB_TransmitString("No input received. Using default (Short).\r\n");
        g_config.protocol_logging_verbose = 0;
    }
}


/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Display baudrate options
  * @retval None
  */
static void CONFIG_DisplayBaudrateOptions(void)
{
    USB_TransmitString("1. 9600\r\n");
    USB_TransmitString("2. 19200\r\n");
    USB_TransmitString("3. 38400\r\n");
    USB_TransmitString("4. 57600\r\n");
    USB_TransmitString("5. 115200\r\n");
}

/**
  * @brief  Display parity options
  * @retval None
  */
static void CONFIG_DisplayParityOptions(void)
{
    USB_TransmitString("1. None\r\n");
    USB_TransmitString("2. Even\r\n");
    USB_TransmitString("3. Odd\r\n");
}

/**
  * @brief  Display protocol options
  * @retval None
  */
static void CONFIG_DisplayProtocolOptions(void)
{
    USB_TransmitString("1. ID003\r\n");
    USB_TransmitString("2. CCTalk\r\n");
}

/**
  * @brief  Display bill table in binary format
  * @retval None
  */
static void CONFIG_DisplayBillTableBinary(void)
{
    for (int i = 0; i < 8; i++)
    {
        USB_TransmitString(g_config.bill_table[i] ? "1" : "0");
    }
    USB_TransmitString("\r\n");
}


/**
  * @brief  Display separator line
  * @retval None
  */
static void CONFIG_DisplaySeparator(void)
{
    USB_TransmitString("--------------------------------\r\n");
}

/**
  * @brief  Display "Enter choice" message with specified range
  * @param  max_choice: Maximum choice number (e.g., 12 for "Enter choice (1-12)")
  * @retval None
  */
static void CONFIG_DisplayEnterChoice(uint8_t max_choice)
{
    USB_TransmitString("\r\nEnter choice (1-");
    
    // Convert max_choice to string and display
    char num_str[4]; // Buffer for number string (max 3 digits + null)
    uint8_t pos = 0;
    uint8_t temp = max_choice;
    
    // Handle zero case
    if (temp == 0)
    {
        num_str[pos++] = '0';
    }
    else
    {
        // Extract digits in reverse order
        uint8_t digits[3];
        uint8_t digit_count = 0;
        
        while (temp > 0)
        {
            digits[digit_count++] = temp % 10;
            temp /= 10;
        }
        
        // Write digits in correct order
        for (int8_t i = digit_count - 1; i >= 0; i--)
        {
            num_str[pos++] = '0' + digits[i];
        }
    }
    
    num_str[pos] = '\0';
    USB_TransmitString(num_str);
    USB_TransmitString("): ");
    HAL_Delay(100); // Give time for message to send2
    USB_Flush();
}




/**
  * @brief  Display protocol name
  * @param  protocol: protocol type
  * @retval None
  */
static void CONFIG_DisplayProtocol(uint8_t protocol)
{
    switch (protocol)
    {
        case PROTO_CCNET:
            USB_TransmitString("CCNET");
            break;
        case PROTO_ID003:
            USB_TransmitString("ID003");
            break;
        case PROTO_CCTALK:
            USB_TransmitString("CCTalk");
            break;
        default:
            USB_TransmitString("Unknown");
            break;
    }
}

/**
  * @brief  Display baudrate value
  * @param  baudrate: baudrate value
  * @retval None
  */
static void CONFIG_DisplayBaudrate(uint32_t baudrate)
{
    if (baudrate == 9600) USB_TransmitString("9600");
    else if (baudrate == 19200) USB_TransmitString("19200");
    else if (baudrate == 38400) USB_TransmitString("38400");
    else if (baudrate == 57600) USB_TransmitString("57600");
    else if (baudrate == 115200) USB_TransmitString("115200");
    else USB_TransmitString("9600");
}

/**
  * @brief  Display parity name
  * @param  parity: parity type
  * @retval None
  */
static void CONFIG_DisplayParity(uint32_t parity)
{
    switch (parity)
    {
        case UART_PARITY_NONE:
            USB_TransmitString("None");
            break;
        case UART_PARITY_EVEN:
            USB_TransmitString("Even");
            break;
        case UART_PARITY_ODD:
            USB_TransmitString("Odd");
            break;
        default:
            USB_TransmitString("Unknown");
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
    USB_TransmitString("Exiting configuration menu...\r\n");
    USB_TransmitString("Restarting MCU...\r\n");
    
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
    
    uint32_t offset = 0;
    
    /* Serialize upstream interface configuration */
    if (g_config.upstream != NULL)
    {
        CONFIG_MemCpy(&buffer[offset], (const uint8_t*)g_config.upstream, sizeof(interface_config_t));
        offset += sizeof(interface_config_t);
    }
    else
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Serialize downstream interface configuration */
    if (g_config.downstream != NULL)
    {
        CONFIG_MemCpy(&buffer[offset], (const uint8_t*)g_config.downstream, sizeof(interface_config_t));
        offset += sizeof(interface_config_t);
    }
    else
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Serialize other configuration fields */
    buffer[offset++] = g_config.usb_logging_enabled;
    buffer[offset++] = g_config.protocol_logging_verbose;
    
    /* Serialize bill table */
    for (int i = 0; i < 8; i++)
    {
        buffer[offset++] = g_config.bill_table[i];
    }
    
    *buffer_size = offset;
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
    
    /* Calculate expected buffer size: 2x interface_config_t + 2 bytes + 8 bytes bill table */
    uint32_t expected_size = (2 * sizeof(interface_config_t)) + 2 + 8;
    
    if (buffer_size != expected_size)
    {
        LOG_Error("Buffer size does not match expected config size");
        return NVM_INVALID_PARAM;
    }
    
    uint32_t offset = 0;
    
    /* Deserialize upstream interface configuration */
    if (g_config.upstream != NULL)
    {
        CONFIG_MemCpy((uint8_t*)g_config.upstream, &buffer[offset], sizeof(interface_config_t));
        offset += sizeof(interface_config_t);
    }
    else
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Deserialize downstream interface configuration */
    if (g_config.downstream != NULL)
    {
        CONFIG_MemCpy((uint8_t*)g_config.downstream, &buffer[offset], sizeof(interface_config_t));
        offset += sizeof(interface_config_t);
    }
    else
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Deserialize other configuration fields */
    g_config.usb_logging_enabled = buffer[offset++];
    g_config.protocol_logging_verbose = buffer[offset++];
    
    /* Deserialize bill table */
    for (int i = 0; i < 8; i++)
    {
        g_config.bill_table[i] = buffer[offset++];
    }
    
    return NVM_OK;
}

/**
  * @brief  Copy memory from source to destination
  * @param  dest: Destination buffer
  * @param  src: Source buffer
  * @param  length: Number of bytes to copy
  * @retval None
  */
static void CONFIG_MemCpy(uint8_t* dest, const uint8_t* src, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        dest[i] = src[i];
    }
}
