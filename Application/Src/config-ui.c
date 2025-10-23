/**
  ******************************************************************************
  * @file           : config-ui.c
  * @brief          : Configuration UI module implementation
  *                   Menu interface and user interaction for configuration
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "config-ui.h"
#include "config.h"
#include "usb.h"
#include "log.h"
#include "led.h"
#include "table-ui.h"
#include <stdio.h>  /* For snprintf */
#include <string.h> /* For strlen */

/* External LED handle */
extern LED_HandleTypeDef hled3;

/* State variable to track if we're in a menu function */
static uint8_t in_menu_function = 0;

/* Private function prototypes -----------------------------------------------*/
static void DisplayBaudrateOptions(void);
static void DisplayParityOptions(void);
static void DisplayProtocolOptions(void);
static void DisplayBillTableBinary(void);
static void DisplayProtocol(uint8_t protocol);
static void DisplayBaudrate(uint32_t baudrate);
static void DisplayParity(uint32_t parity);
static void DisplayInterfaceSettings(const char* name, interface_config_t* interface);
static void UpdateUpstreamProtocol(void);
static void UpdateUpstreamBaudrate(void);
static void UpdateUpstreamParity(void);
static void UpdateDownstreamProtocol(void);
static void UpdateCCTalkAddresses(void);
static void UpdateDownstreamBaudrate(void);
static void UpdateDownstreamParity(void);
static void UpdateDownstreamPolling(void);
static void ShowBillTable(void);
static void UpdateUsbLogging(void);
static void UpdateProtocolLogging(void);
static void DisplaySeparator(void);
static void DisplayEnterChoice(uint8_t max_choice);
static uint8_t WaitForInput(void);
static uint8_t ParseChoice(const char* input, uint8_t min, uint8_t max);
static void ExitMenu(void);

/* Exported functions --------------------------------------------------------*/

void CONFIGUI_ShowMenu(void)
{
    CONFIGUI_ShowConfiguration();
    HAL_Delay(100); /* 100ms delay to ensure the configuration is displayed */
    
    USB_TransmitString("12. Exit and Restart\r\n");
    USB_TransmitString("13. Save, Exit and Restart\r\n");
    USB_TransmitString("======================================================\r\n");
    DisplayEnterChoice(13);
    HAL_Delay(100);
    USB_Flush();
}

/**
  * @brief  Show configuration menu
  * @retval None
  */
void CONFIGUI_ShowConfiguration(void)
{
    char config_line[128];
    
    USB_TransmitString("\r\n=== CONFIGURATION ====================================\r\n");
    USB_TransmitString("1.  Upstream Protocol        : CCNET - fixed\r\n");
    
    USB_TransmitString("2.  Upstream Baudrate        : ");
    DisplayBaudrate(g_config.upstream->phy.baudrate);
    USB_TransmitString("\r\n");
    
    USB_TransmitString("3.  Upstream Parity          : ");
    DisplayParity(g_config.upstream->phy.parity);
    USB_TransmitString("\r\n");
    
    USB_TransmitString("4.  Downstream Protocol      : ");
    DisplayProtocol(g_config.downstream->protocol);
    USB_TransmitString("\r\n");
    
    USB_TransmitString("5.  ccTalk Addresses         : ");
    snprintf(config_line, sizeof(config_line), "%d, %d\r\n", 
             g_config.downstream->datalink.cctalk_dest_address,
             g_config.downstream->datalink.cctalk_source_address);
    USB_TransmitString(config_line);
    
    USB_TransmitString("6.  Downstream Baudrate      : ");
    DisplayBaudrate(g_config.downstream->phy.baudrate);
    USB_TransmitString("\r\n");
    
    USB_TransmitString("7.  Downstream Parity        : ");
    DisplayParity(g_config.downstream->phy.parity);
    USB_TransmitString("\r\n");
    
    USB_TransmitString("8.  Downstream Polling       : ");
    USB_TransmitString(g_config.downstream->datalink.polling_period_ms == 0 ? "Disabled  (synchronous)" :
                       g_config.downstream->datalink.polling_period_ms == 100 ? "100ms (asynchronous)" : 
                       g_config.downstream->datalink.polling_period_ms == 200 ? "200ms (asynchronous)" : 
                       g_config.downstream->datalink.polling_period_ms == 500 ? "500ms (asynchronous)" : 
                       g_config.downstream->datalink.polling_period_ms == 1000 ? "1000ms (asynchronous)" : "Custom");
    USB_TransmitString("\r\n");
    
    USB_TransmitString("9.  Show Bill Table\r\n");
    
    snprintf(config_line, sizeof(config_line), "10. USB Logging              : %s\r\n", 
             g_config.usb_logging_enabled ? "Enabled" : "Disabled");
    USB_TransmitString(config_line);
    
    const char* log_level_str;
    switch (g_config.log_level)
    {
        case LOG_LEVEL_ERROR: log_level_str = "ERROR"; break;
        case LOG_LEVEL_WARN:  log_level_str = "WARN"; break;
        case LOG_LEVEL_PROTO: log_level_str = "PROTO"; break;
        case LOG_LEVEL_INFO:  log_level_str = "INFO"; break;
        case LOG_LEVEL_DEBUG: log_level_str = "DEBUG"; break;
        default:              log_level_str = "INFO"; break;
    }
    snprintf(config_line, sizeof(config_line), "11. Log Level                : %s\r\n", log_level_str);
    USB_TransmitString(config_line);
    USB_TransmitString("======================================================\r\n\r\n");
}

/**
  * @brief  Process configuration menu
  * @retval None
  */
void CONFIGUI_ProcessMenu(void)
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
            uint8_t choice = ParseChoice(input_buffer, 1, 13);
            
            // Process the choice
            if (choice > 0)
            {
                switch (choice)
                {
                    case CONFIGUI_MENU_UPSTREAM_PROTOCOL:
                        UpdateUpstreamProtocol();
                        break;
                    case CONFIGUI_MENU_UPSTREAM_BAUDRATE:
                        UpdateUpstreamBaudrate();
                        break;
                    case CONFIGUI_MENU_UPSTREAM_PARITY:
                        UpdateUpstreamParity();
                        break;
                    case CONFIGUI_MENU_DOWNSTREAM_PROTOCOL:
                        UpdateDownstreamProtocol();
                        break;
                    case CONFIGUI_MENU_CCTALK_ADDRESSES:
                        UpdateCCTalkAddresses();
                        break;
                    case CONFIGUI_MENU_DOWNSTREAM_BAUDRATE:
                        UpdateDownstreamBaudrate();
                        break;
                    case CONFIGUI_MENU_DOWNSTREAM_PARITY:
                        UpdateDownstreamParity();
                        break;
                    case CONFIGUI_MENU_DOWNSTREAM_POLLING:
                        UpdateDownstreamPolling();
                        break;
                    case CONFIGUI_MENU_SHOW_BILL_TABLE:
                        ShowBillTable();
                        break;
                    case CONFIGUI_MENU_USB_LOGGING:
                        UpdateUsbLogging();
                        break;
                    case CONFIGUI_MENU_LOG_LEVEL:
                        UpdateProtocolLogging();
                        break;
                    case CONFIGUI_MENU_EXIT:
                        ExitMenu();
                        return; // Exit immediately, don't show menu again
                    case CONFIGUI_MENU_SAVE_EXIT:
                        CONFIG_SaveToNVM();
                        ExitMenu();
                        return; // Exit immediately, don't show menu again
                    default:
                        USB_TransmitString("Invalid choice!\r\n");
                        break;
                }
                
                // Show menu again after processing (except for exit)
                CONFIGUI_ShowMenu();
            }
            else
            {
                USB_TransmitString("Invalid choice! Please enter a number between 1 and 13: ");
            }
        }
    }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Display interface settings
  * @param  name: interface name
  * @param  interface: interface configuration pointer
  * @retval None
  */
static void DisplayInterfaceSettings(const char* name, interface_config_t* interface)
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
        if (interface->datalink.polling_period_ms == 0)
            USB_TransmitString("Disabled");
        else if (interface->datalink.polling_period_ms == 50)
            USB_TransmitString("50ms");
        else if (interface->datalink.polling_period_ms == 100)
            USB_TransmitString("100ms");
        else if (interface->datalink.polling_period_ms == 200)
            USB_TransmitString("200ms");
        else if (interface->datalink.polling_period_ms == 500)
            USB_TransmitString("500ms");
        else if (interface->datalink.polling_period_ms == 1000)
            USB_TransmitString("1000ms");
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
static void UpdateUpstreamProtocol(void)
{
    USB_TransmitString("Upstream protocol is fixed to CCNET and cannot be changed.\r\n");
}

/**
  * @brief  Update upstream baudrate
  * @retval None
  */
static void UpdateUpstreamBaudrate(void)
{
    USB_TransmitString("\r\nSelect upstream baudrate:\r\n");
    DisplayBaudrateOptions();
    DisplayEnterChoice(5);
    
    // Wait for user input
    WaitForInput();
    char input_buffer[16];
    if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
    {
        uint8_t choice = ParseChoice(input_buffer, 1, 5);
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
static void UpdateUpstreamParity(void)
{
    USB_TransmitString("\r\nSelect upstream parity:\r\n");
    DisplayParityOptions();
    DisplayEnterChoice(3);
    
    // Wait for user input
    WaitForInput();
    char input_buffer[16];
    if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
    {
        uint8_t choice = ParseChoice(input_buffer, 1, 3);
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
static void UpdateDownstreamProtocol(void)
{
    USB_TransmitString("\r\nSelect downstream protocol:\r\n");
    DisplayProtocolOptions();
    DisplayEnterChoice(2);
    
    // Wait for user input
    WaitForInput();
    char input_buffer[16];
    if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
    {
        uint8_t choice = ParseChoice(input_buffer, 1, 2);
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
  * @brief  Update ccTalk addresses
  * @retval None
  */
static void UpdateCCTalkAddresses(void)
{
    USB_TransmitString("\r\nccTalk Address Configuration:\r\n");
    
    // Prompt for destination address first
    USB_TransmitString("Enter destination address (0-255, 0 for broadcast): ");
    if (WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t dest_addr = ParseChoice(input_buffer, 0, 255);
            g_config.downstream->datalink.cctalk_dest_address = dest_addr;
            USB_TransmitString("Destination address updated.\r\n");
        }
        else
        {
            USB_TransmitString("No input received. Using default (0).\r\n");
            g_config.downstream->datalink.cctalk_dest_address = 0;
        }
    }
    
    // Prompt for source address next
    USB_TransmitString("Enter source address (1-255): ");
    if (WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t source_addr = ParseChoice(input_buffer, 0, 255);
            if (source_addr == 0)
            {
                source_addr = 1;  // Convert 0 to 1
                USB_TransmitString("Invalid address! Using default (1).\r\n");
            }
            g_config.downstream->datalink.cctalk_source_address = source_addr;
            USB_TransmitString("Source address updated.\r\n");
        }
        else
        {
            USB_TransmitString("No input received. Using default (1).\r\n");
            g_config.downstream->datalink.cctalk_source_address = 1;
        }
    }
}

/**
  * @brief  Update downstream baudrate
  * @retval None
  */
static void UpdateDownstreamBaudrate(void)
{
    USB_TransmitString("\r\nSelect downstream baudrate:\r\n");
    DisplayBaudrateOptions();
    DisplayEnterChoice(5);
    
    // Wait for user input
    if (WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t choice = ParseChoice(input_buffer, 1, 5);
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
static void UpdateDownstreamParity(void)
{
    USB_TransmitString("\r\nSelect downstream parity:\r\n");
    DisplayParityOptions();
    DisplayEnterChoice(3);
    
    // Wait for user input
    if (WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t choice = ParseChoice(input_buffer, 1, 3);
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
static void UpdateDownstreamPolling(void)
{
    USB_TransmitString("\r\nSelect downstream polling period:\r\n");
    USB_TransmitString("1. Disabled\r\n");
    USB_TransmitString("2. 100ms\r\n");
    USB_TransmitString("3. 200ms\r\n");
    USB_TransmitString("4. 500ms\r\n");
    USB_TransmitString("5. 1000ms\r\n");
    DisplayEnterChoice(5);
    
    // Wait for user input
    if (WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t choice = ParseChoice(input_buffer, 1, 5);
            if (choice > 0)
            {
                switch (choice)
                {
                    case 1: g_config.downstream->datalink.polling_period_ms = 0; break;
                    case 2: g_config.downstream->datalink.polling_period_ms = 100; break;
                    case 3: g_config.downstream->datalink.polling_period_ms = 200; break;
                    case 4: g_config.downstream->datalink.polling_period_ms = 500; break;
                    case 5: g_config.downstream->datalink.polling_period_ms = 1000; break;
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
  * @brief  Show bill table
  * @retval None
  */
static void ShowBillTable(void)
{
    TABLE_UI_DisplayBillTable();
    USB_Flush();
}

/**
  * @brief  Update USB logging setting
  * @retval None
  */
static void UpdateUsbLogging(void)
{
    USB_TransmitString("\r\nUSB Logging: ");
    USB_TransmitString(g_config.usb_logging_enabled ? "Enabled" : "Disabled");
    USB_TransmitString("\r\n");
    USB_TransmitString("1. Enable\r\n");
    USB_TransmitString("2. Disable\r\n");
    DisplayEnterChoice(2);
    
    // Wait for user input
    if (WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t choice = ParseChoice(input_buffer, 1, 2);
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
static void UpdateProtocolLogging(void)
{
    USB_TransmitString("\r\nCurrent Log Level: ");
    switch (g_config.log_level)
    {
        case LOG_LEVEL_ERROR: USB_TransmitString("ERROR"); break;
        case LOG_LEVEL_WARN:  USB_TransmitString("WARN"); break;
        case LOG_LEVEL_PROTO: USB_TransmitString("PROTO"); break;
        case LOG_LEVEL_INFO:  USB_TransmitString("INFO"); break;
        case LOG_LEVEL_DEBUG: USB_TransmitString("DEBUG"); break;
        default:              USB_TransmitString("INFO"); break;
    }
    USB_TransmitString("\r\n");
    USB_TransmitString("1. ERROR\r\n");
    USB_TransmitString("2. WARN\r\n");
    USB_TransmitString("3. PROTO\r\n");
    USB_TransmitString("4. INFO\r\n");
    USB_TransmitString("5. DEBUG\r\n");
    DisplayEnterChoice(5);
    
    // Wait for user input
    if (WaitForInput())
    {
        char input_buffer[16];
        if (USB_GetInputLine(input_buffer, sizeof(input_buffer)) > 0)
        {
            uint8_t choice = ParseChoice(input_buffer, 1, 5);
            if (choice > 0)
            {
                switch (choice)
                {
                    case 1: g_config.log_level = LOG_LEVEL_ERROR; break;
                    case 2: g_config.log_level = LOG_LEVEL_WARN;  break;
                    case 3: g_config.log_level = LOG_LEVEL_PROTO; break;
                    case 4: g_config.log_level = LOG_LEVEL_INFO;  break;
                    case 5: g_config.log_level = LOG_LEVEL_DEBUG; break;
                }
                LOG_SetLevel(g_config.log_level);
            }
            else
            {
                USB_TransmitString("Invalid choice! Using default (INFO).\r\n");
                g_config.log_level = LOG_LEVEL_INFO;
            }
        }
        else
        {
            USB_TransmitString("No input received. Using default (INFO).\r\n");
            g_config.log_level = LOG_LEVEL_INFO;
        }
    }
    else
    {
        USB_TransmitString("No input received. Using default (INFO).\r\n");
        g_config.log_level = LOG_LEVEL_INFO;
    }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Display baudrate options
  * @retval None
  */
static void DisplayBaudrateOptions(void)
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
static void DisplayParityOptions(void)
{
    USB_TransmitString("1. None\r\n");
    USB_TransmitString("2. Even\r\n");
    USB_TransmitString("3. Odd\r\n");
}

/**
  * @brief  Display protocol options
  * @retval None
  */
static void DisplayProtocolOptions(void)
{
    USB_TransmitString("1. ID003\r\n");
    USB_TransmitString("2. CCTalk\r\n");
}

/**
  * @brief  Display bill table in binary format
  * @retval None
  */
static void DisplayBillTableBinary(void)
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
static void DisplaySeparator(void)
{
    USB_TransmitString("--------------------------------\r\n");
}

/**
  * @brief  Display "Enter choice" message with specified range
  * @param  max_choice: Maximum choice number (e.g., 12 for "Enter choice (1-12)")
  * @retval None
  */
static void DisplayEnterChoice(uint8_t max_choice)
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
static void DisplayProtocol(uint8_t protocol)
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
static void DisplayBaudrate(uint32_t baudrate)
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
static void DisplayParity(uint32_t parity)
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
static uint8_t WaitForInput(void)
{
    USB_Flush();
    HAL_Delay(10);
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
static uint8_t ParseChoice(const char* input, uint8_t min, uint8_t max)
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
static void ExitMenu(void)
{
    USB_TransmitString("Exiting configuration menu...\r\n");
    USB_TransmitString("Restarting MCU...\r\n");
    
    // Restart the MCU
    HAL_Delay(100);  // Give time for USB transmission to complete
    HAL_NVIC_SystemReset();
}
