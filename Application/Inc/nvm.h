/**
  ******************************************************************************
  * @file           : nvm.h
  * @brief          : Non-volatile memory module header file
  *                   Generic flash interface and config-specific storage
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __NVM_H
#define __NVM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "config.h"
#include "crc.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  NVM operation result enumeration
  */
typedef enum
{
    NVM_OK = 0,
    NVM_ERROR,
    NVM_INVALID_PARAM,
    NVM_WRITE_FAILED,
    NVM_READ_FAILED,
    NVM_ERASE_FAILED,
    NVM_CRC_ERROR,
    NVM_CORRUPTED_DATA
} nvm_result_t;

/* Exported constants --------------------------------------------------------*/

/* Flash configuration storage addresses */
#define FLASH_CONFIG_BANK1     0x0801F000U  /* Page 126 */
#define FLASH_CONFIG_BANK2     0x0801F800U  /* Page 127 */
#define NVM_PAGE_SIZE          2048         /* Flash page size in bytes */

/* Configuration storage structure */
#define CONFIG_MAGIC_NUMBER    0x12345678   /* Magic number for validation */
#define CONFIG_VERSION         1            /* Configuration version */

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/* Public NVM Interface - Raw data management with bank selection and verification */
nvm_result_t NVM_Init(void);
nvm_result_t NVM_ReadConfigData(uint8_t* data, uint32_t max_size, uint32_t* actual_size);
nvm_result_t NVM_WriteConfigData(const uint8_t* data, uint32_t data_size);
uint32_t NVM_GetCurrentSequenceNumber(void);

/* Note: Generic flash functions (EraseFlashPage, WriteFlash, ReadFlash, EraseConfig) 
 *       are internal implementation details and not part of the public API */

#ifdef __cplusplus
}
#endif

#endif /* __NVM_H */