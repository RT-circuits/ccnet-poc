/**
  ******************************************************************************
  * @file           : table-ui.c
  * @brief          : Bill table UI module implementation
  *                   Display functions for bill table
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "table-ui.h"
#include "app.h"
#include "usb.h"
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define BUFFER_SIZE 150

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void TABLE_UI_DisplayHeader(void);
static void TABLE_UI_DisplayRow(uint8_t ccnet_bit, const char* currency, uint16_t value, uint8_t id003_denom, uint8_t country_code);
static void TABLE_UI_DisplaySeparator(void);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Display bill table in formatted columns
  * @retval None
  */
void TABLE_UI_DisplayBillTable(void)
{
    char buffer[BUFFER_SIZE];
    
    /* Display title */
    USB_TransmitString("\r\n=== BILL TABLE RETRIEVED FROM DOWNSTREAM VALIDATOR ===\r\n");
    USB_TransmitString("|-------------------------------------------------|\r\n");
    
    /* Check if table is loaded */
    if (g_bill_table.is_loaded == 0)
    {
        USB_TransmitString("Bill table not loaded from downstream validator\r\n");
        USB_TransmitString("==================\r\n\r\n");
        return;
    }
    
    /* Display table header */
    TABLE_UI_DisplayHeader();
    TABLE_UI_DisplaySeparator();
    
    /* Display each non-zero denomination */
    for (uint8_t i = 0; i < g_bill_table.count && i < MAX_BILL_DENOMS; i++)
    {
        if (g_bill_table.denoms[i].value > 0)
        {
            TABLE_UI_DisplayRow(
                g_bill_table.denoms[i].ccnet_bitnr,
                g_bill_table.currency,
                g_bill_table.denoms[i].value,
                g_bill_table.denoms[i].id003_denom_nr,
                g_bill_table.denoms[i].country_code
            );
        }
    }
    
    TABLE_UI_DisplaySeparator();
    
    /* Display summary */
    snprintf(buffer, BUFFER_SIZE, "Total denominations: %d\r\n", g_bill_table.count);
    USB_TransmitString(buffer);
    USB_TransmitString("======================================================\r\n\r\n");
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Display table header
  * @retval None
  */
static void TABLE_UI_DisplayHeader(void)
{
    USB_TransmitString("|              CCNET           ||    Downstream   |\r\n");
    USB_TransmitString("| Bill Type | Value | Currency || Denom | Country |\r\n");
}

/**
  * @brief  Display separator line
  * @retval None
  */
static void TABLE_UI_DisplaySeparator(void)
{
    USB_TransmitString("|-----------|-------|----------||-------|---------|\r\n");
}

/**
  * @brief  Display a single table row
  * @param  ccnet_bit: CCNET bit number (0-23)
  * @param  currency: Currency code (e.g., "EUR")
  * @param  value: Denomination value
  * @param  id003_denom: ID003 denomination number
  * @param  country_code: Country code
  * @retval None
  */
static void TABLE_UI_DisplayRow(uint8_t ccnet_bit, const char* currency, uint16_t value, uint8_t id003_denom, uint8_t country_code)
{
    char buffer[BUFFER_SIZE];
    
    /* Format: "|    xx     |  xxxx | CUR      | 0xXX  | 0xXX" */
    snprintf(buffer, BUFFER_SIZE, "|    %2d     | %5d | %-8s || 0x%02X  | 0x%02X    |\r\n",
             ccnet_bit,
             value,
             currency,
             id003_denom,
             country_code);
    
    USB_TransmitString(buffer);
}

