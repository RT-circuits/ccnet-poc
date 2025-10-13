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
#include "log.h"

/* Private defines -----------------------------------------------------------*/
#define BUFFER_SIZE 150

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void TABLE_UI_DisplayHeader(void);
static void TABLE_UI_DisplayRow(uint8_t ccnet_bit, const char* currency, uint16_t value, uint8_t id003_denom, uint8_t country_code);
static char TABLE_UI_GetEnabledStatus(uint32_t enabled_bills, uint32_t escrowed_bills, uint8_t bit);
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
    USB_TransmitString("\r\n== BILL TABLE ========================================================\r\n");
    USB_TransmitString("|-------------------------------------------------------------------|\r\n");
    
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
    
    /* Display legend */
    USB_TransmitString("Bill Type Status: N = not enabled, Y = enabled, E = enabled with Escrow\r\n");
    USB_TransmitString("======================================================================\r\n\r\n");

    snprintf(buffer, BUFFER_SIZE, "g_bill_table.enabled_bills: 0x%02X, g_bill_table.escrowed: 0x%02X, g_bill_table.ds_enabled_bills: 0x%02X, g_bill_table.ds_escrowed_bills: 0x%02X", 
      (uint8_t)g_bill_table.enabled_bills, (uint8_t)g_bill_table.escrowed_bills, (uint8_t)g_bill_table.ds_enabled_bills, (uint8_t)  g_bill_table.ds_escrowed_bills);
    LOG_Debug(buffer);
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Display table header
  * @retval None
  */
static void TABLE_UI_DisplayHeader(void)
{
    USB_TransmitString("|              CCNET           ||    Downstream   ||  Bill status   |\r\n");
    USB_TransmitString("| Bill Type | Value | Currency || Denom | Country || CCNET | Downs. |\r\n");
}

/**
  * @brief  Display separator line
  * @retval None
  */
static void TABLE_UI_DisplaySeparator(void)
{
    USB_TransmitString("|-----------|-------|----------||-------|---------||-------|--------|\r\n");
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
    char ccnet_status = TABLE_UI_GetEnabledStatus(g_bill_table.enabled_bills, g_bill_table.escrowed_bills, ccnet_bit);
    char ds_status = TABLE_UI_GetEnabledStatus(g_bill_table.ds_enabled_bills, g_bill_table.ds_escrowed_bills, ccnet_bit);
    
    /* Format: "|    xx     |  xxxx | CUR      | 0xXX  | 0xXX    |  X   |   X   |" */
    snprintf(buffer, BUFFER_SIZE, "|    %2d     | %5d | %-8s || 0x%02X  | 0x%02X    ||   %c   |    %c   |\r\n",
             ccnet_bit,
             value,
             currency,
             id003_denom,
             country_code,
             ccnet_status,
             ds_status);
    
    USB_TransmitString(buffer);
}

/**
  * @brief  Get enabled status character for a bill type
  * @param  enabled_bills: Bitmap of enabled bills
  * @param  escrowed_bills: Bitmap of escrowed bills
  * @param  bit: Bit position to check
  * @retval char: 'N' = not enabled, 'Y' = enabled, 'E' = enabled with escrow
  */
static char TABLE_UI_GetEnabledStatus(uint32_t enabled_bills, uint32_t escrowed_bills, uint8_t bit)
{
    uint32_t bit_mask = 1UL << bit;
    
    if (!(enabled_bills & bit_mask))
    {
        return 'N';  /* Not enabled */
    }
    else if (escrowed_bills & bit_mask)
    {
        return 'E';  /* Enabled with escrow */
    }
    else
    {
        return 'Y';  /* Enabled without escrow */
    }
}

