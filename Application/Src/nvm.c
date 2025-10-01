/**
  ******************************************************************************
  * @file           : nvm.c
  * @brief          : Non-volatile memory module implementation
  *                   Generic flash interface and config-specific storage
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "nvm.h"
#include "log.h"

/* Private variables ---------------------------------------------------------*/

/* Generic data storage structure */
typedef struct
{
    uint32_t magic;         /* Magic number for validation */
    uint32_t version;       /* Configuration version */
    uint32_t sequence;      /* Sequence number for bank management */
    uint32_t data_size;     /* Size of data payload */
    uint8_t data[512];      /* Generic data payload (max 512 bytes) */
    uint16_t crc16;         /* 16-bit CRC for integrity verification */
    uint16_t padding;       /* Padding to ensure 4-byte alignment */
} nvm_data_storage_t;

/* Private function prototypes -----------------------------------------------*/
static nvm_result_t NVM_EraseFlashPage(uint32_t address);
static nvm_result_t NVM_WriteFlash(uint32_t address, const uint8_t* data, uint32_t length);
static nvm_result_t NVM_ReadFlash(uint32_t address, uint8_t* data, uint32_t length);
static nvm_result_t NVM_EraseConfig(void);
static nvm_result_t NVM_UnlockFlash(void);
static nvm_result_t NVM_LockFlash(void);
static uint32_t NVM_GetSequenceNumber(uint32_t address);
static nvm_result_t NVM_ValidateStorage(const nvm_data_storage_t* storage);
static uint16_t NVM_CalculateCRC16(const uint8_t* data, uint32_t length);
static nvm_result_t NVM_WriteDataToBank(uint32_t address, const uint8_t* data, uint32_t data_size, uint32_t sequence);
static nvm_result_t NVM_ReadDataFromBank(uint32_t address, uint8_t* data, uint32_t max_size, uint32_t* actual_size);
static void NVM_MemCpy(uint8_t* dest, const uint8_t* src, uint32_t length);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize NVM module
  * @retval NVM operation result
  */
nvm_result_t NVM_Init(void)
{
    /* Initialize flash interface */
    if (NVM_UnlockFlash() != NVM_OK)
    {
        LOG_Error("Failed to unlock flash for NVM initialization");
        return NVM_ERROR;
    }
    
    return NVM_OK;
}

/**
  * @brief  Erase a flash page (private function)
  * @param  address: Flash page address to erase
  * @retval NVM operation result
  */
static nvm_result_t NVM_EraseFlashPage(uint32_t address)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error;
    HAL_StatusTypeDef status;
    
    /* Verify address is page-aligned */
    if ((address % NVM_PAGE_SIZE) != 0)
    {
        LOG_Error("Flash address not page-aligned");
        return NVM_INVALID_PARAM;
    }
    
    /* Unlock flash */
    if (NVM_UnlockFlash() != NVM_OK)
    {
        return NVM_ERROR;
    }
    
    /* Calculate page number */
    uint32_t page_number = (address - FLASH_BASE) / NVM_PAGE_SIZE;
    
    /* Configure erase operation */
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Page = page_number;
    erase_init.NbPages = 1;
    
    /* Erase the page */
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    
    /* Lock flash */
    NVM_LockFlash();
    
    if (status != HAL_OK)
    {
        LOG_Error("Flash erase failed");
        return NVM_ERASE_FAILED;
    }
    
    return NVM_OK;
}

/**
  * @brief  Write data to flash (private function)
  * @param  address: Flash address to write to
  * @param  data: Pointer to data to write
  * @param  length: Number of bytes to write
  * @retval NVM operation result
  */
static nvm_result_t NVM_WriteFlash(uint32_t address, const uint8_t* data, uint32_t length)
{
    if (data == NULL || length == 0)
    {
        LOG_Error("Invalid parameters for flash write");
        return NVM_INVALID_PARAM;
    }
    
    /* Unlock flash */
    if (NVM_UnlockFlash() != NVM_OK)
    {
        return NVM_ERROR;
    }
    
    /* Write data in 64-bit double words */
    const uint64_t* src = (const uint64_t*)data;
    uint32_t dest_addr = address;
    uint32_t double_words = (length + 7) / 8; /* Round up to double word boundary */
    
    for (uint32_t i = 0; i < double_words; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, dest_addr, *src) != HAL_OK)
        {
            NVM_LockFlash();
            LOG_Error("Flash write failed");
            return NVM_WRITE_FAILED;
        }
        dest_addr += 8;
        src++;
    }
    
    /* Lock flash */
    NVM_LockFlash();
    
    return NVM_OK;
}

/**
  * @brief  Read data from flash (private function)
  * @param  address: Flash address to read from
  * @param  data: Pointer to buffer to store read data
  * @param  length: Number of bytes to read
  * @retval NVM operation result
  */
static nvm_result_t NVM_ReadFlash(uint32_t address, uint8_t* data, uint32_t length)
{
    if (data == NULL || length == 0)
    {
        LOG_Error("Invalid parameters for flash read");
        return NVM_INVALID_PARAM;
    }
    
    /* Copy data from flash */
    NVM_MemCpy(data, (const uint8_t*)address, length);
    
    return NVM_OK;
}

/**
  * @brief  Read data from NVM with bank selection and verification
  * @param  data: Pointer to buffer to fill with data
  * @param  max_size: Maximum size of the buffer
  * @param  actual_size: Pointer to receive actual data size read
  * @retval NVM operation result
  */
nvm_result_t NVM_ReadConfigData(uint8_t* data, uint32_t max_size, uint32_t* actual_size)
{
    nvm_data_storage_t bank1_storage, bank2_storage;
    nvm_result_t bank1_result, bank2_result;
    uint32_t bank1_sequence, bank2_sequence;
    nvm_result_t result;
    
    if (data == NULL || actual_size == NULL)
    {
        LOG_Error("Invalid parameter for data read");
        return NVM_INVALID_PARAM;
    }
    
    /* Read both banks */
    bank1_result = NVM_ReadFlash(FLASH_CONFIG_BANK1, (uint8_t*)&bank1_storage, sizeof(bank1_storage));
    bank2_result = NVM_ReadFlash(FLASH_CONFIG_BANK2, (uint8_t*)&bank2_storage, sizeof(bank2_storage));
    
    /* Get sequence numbers */
    bank1_sequence = NVM_GetSequenceNumber(FLASH_CONFIG_BANK1);
    bank2_sequence = NVM_GetSequenceNumber(FLASH_CONFIG_BANK2);
    
    /* Determine which bank to use */
    uint32_t selected_bank = 0;
    nvm_data_storage_t* selected_storage = NULL;
    
    if (bank1_result == NVM_OK && bank2_result == NVM_OK)
    {
        /* Both banks valid - use the one with higher sequence number */
        if (bank1_sequence >= bank2_sequence)
        {
            selected_bank = FLASH_CONFIG_BANK1;
            selected_storage = &bank1_storage;
        }
        else
        {
            selected_bank = FLASH_CONFIG_BANK2;
            selected_storage = &bank2_storage;
        }
    }
    else if (bank1_result == NVM_OK)
    {
        /* Only bank 1 valid */
        selected_bank = FLASH_CONFIG_BANK1;
        selected_storage = &bank1_storage;
    }
    else if (bank2_result == NVM_OK)
    {
        /* Only bank 2 valid */
        selected_bank = FLASH_CONFIG_BANK2;
        selected_storage = &bank2_storage;
    }
    else
    {
        /* No valid banks found */
        LOG_Error("No valid data found in flash");
        return NVM_ERROR;
    }
    
    /* Validate selected storage */
    result = NVM_ValidateStorage(selected_storage);
    if (result != NVM_OK)
    {
        LOG_Error("Data validation failed for bank");
        
        /* Try the other bank if available */
        if (selected_bank == FLASH_CONFIG_BANK1 && bank2_result == NVM_OK)
        {
            selected_bank = FLASH_CONFIG_BANK2;
            selected_storage = &bank2_storage;
            result = NVM_ValidateStorage(selected_storage);
        }
        else if (selected_bank == FLASH_CONFIG_BANK2 && bank1_result == NVM_OK)
        {
            selected_bank = FLASH_CONFIG_BANK1;
            selected_storage = &bank1_storage;
            result = NVM_ValidateStorage(selected_storage);
        }
        
        if (result != NVM_OK)
        {
            LOG_Error("Both data banks corrupted");
            return NVM_CORRUPTED_DATA;
        }
    }
    
    /* Check if data fits in provided buffer */
    if (selected_storage->data_size > max_size)
    {
        LOG_Error("Data size exceeds buffer size");
        return NVM_INVALID_PARAM;
    }
    
    /* Copy data */
    NVM_MemCpy(data, selected_storage->data, selected_storage->data_size);
    *actual_size = selected_storage->data_size;
    
    return NVM_OK;
}

/**
  * @brief  Write data to NVM with bank selection and verification
  * @param  data: Pointer to data buffer to store
  * @param  data_size: Size of data to store
  * @retval NVM operation result
  */
nvm_result_t NVM_WriteConfigData(const uint8_t* data, uint32_t data_size)
{
    uint32_t bank1_sequence, bank2_sequence;
    uint32_t target_bank;
    uint32_t new_sequence;
    nvm_result_t result;
    
    if (data == NULL || data_size == 0)
    {
        LOG_Error("Invalid parameter for data write");
        return NVM_INVALID_PARAM;
    }
    
    if (data_size > 512)
    {
        LOG_Error("Data size exceeds maximum size");
        return NVM_INVALID_PARAM;
    }
    
    /* Read current sequence numbers */
    bank1_sequence = NVM_GetSequenceNumber(FLASH_CONFIG_BANK1);
    bank2_sequence = NVM_GetSequenceNumber(FLASH_CONFIG_BANK2);
    
    /* Determine target bank (write to the one with lower sequence number) */
    if (bank1_sequence <= bank2_sequence)
    {
        target_bank = FLASH_CONFIG_BANK1;
        new_sequence = bank2_sequence + 1;
    }
    else
    {
        target_bank = FLASH_CONFIG_BANK2;
        new_sequence = bank1_sequence + 1;
    }
    
    /* If both banks are empty (sequence 0), start with sequence 1 */
    if (bank1_sequence == 0 && bank2_sequence == 0)
    {
        new_sequence = 1;
    }
    
    /* Erase target bank */
    result = NVM_EraseFlashPage(target_bank);
    if (result != NVM_OK)
    {
        LOG_Error("Failed to erase target bank");
        return result;
    }
    
    /* Write data to target bank */
    result = NVM_WriteDataToBank(target_bank, data, data_size, new_sequence);
    if (result != NVM_OK)
    {
        LOG_Error("Failed to write data to bank");
        return result;
    }
    
    return NVM_OK;
}

/**
  * @brief  Erase all configuration data (private function)
  * @retval NVM operation result
  */
static nvm_result_t NVM_EraseConfig(void)
{
    nvm_result_t result1, result2;
    
    
    /* Erase both banks */
    result1 = NVM_EraseFlashPage(FLASH_CONFIG_BANK1);
    result2 = NVM_EraseFlashPage(FLASH_CONFIG_BANK2);
    
    if (result1 != NVM_OK || result2 != NVM_OK)
    {
        LOG_Error("Failed to erase configuration banks");
        return NVM_ERROR;
    }
    
    return NVM_OK;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Unlock flash for programming
  * @retval NVM operation result
  */
static nvm_result_t NVM_UnlockFlash(void)
{
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return NVM_ERROR;
    }
    return NVM_OK;
}

/**
  * @brief  Lock flash after programming
  * @retval NVM operation result
  */
static nvm_result_t NVM_LockFlash(void)
{
    if (HAL_FLASH_Lock() != HAL_OK)
    {
        return NVM_ERROR;
    }
    return NVM_OK;
}

/**
  * @brief  Get sequence number from flash address
  * @param  address: Flash address to read sequence from
  * @retval Sequence number (0 if invalid)
  */
static uint32_t NVM_GetSequenceNumber(uint32_t address)
{
    nvm_data_storage_t storage;
    
    if (NVM_ReadFlash(address, (uint8_t*)&storage, sizeof(storage)) != NVM_OK)
    {
        return 0;
    }
    
    /* Check if magic number is valid */
    if (storage.magic != CONFIG_MAGIC_NUMBER)
    {
        return 0;
    }
    
    return storage.sequence;
}

/**
  * @brief  Validate configuration storage structure
  * @param  storage: Pointer to storage structure to validate
  * @retval NVM operation result
  */
static nvm_result_t NVM_ValidateStorage(const nvm_data_storage_t* storage)
{
    if (storage == NULL)
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Check magic number */
    if (storage->magic != CONFIG_MAGIC_NUMBER)
    {
        LOG_Error("Invalid magic number");
        return NVM_CORRUPTED_DATA;
    }
    
    /* Check version */
    if (storage->version != CONFIG_VERSION)
    {
        LOG_Error("Invalid version");
        return NVM_CORRUPTED_DATA;
    }
    
    /* Check data size */
    if (storage->data_size == 0 || storage->data_size > 512)
    {
        LOG_Error("Invalid data size");
        return NVM_CORRUPTED_DATA;
    }
    
    /* Verify CRC */
    uint16_t calculated_crc = NVM_CalculateCRC16(storage->data, storage->data_size);
    if (calculated_crc != storage->crc16)
    {
        LOG_Error("CRC mismatch");
        return NVM_CRC_ERROR;
    }
    
    return NVM_OK;
}

/**
  * @brief  Calculate 16-bit CRC for data
  * @param  data: Pointer to data
  * @param  length: Length of data
  * @retval 16-bit CRC value
  */
static uint16_t NVM_CalculateCRC16(const uint8_t* data, uint32_t length)
{
    /* Use CRC module with CCNET protocol for config data */
    return CRC_Calculate((uint8_t*)data, PROTO_CCNET, (uint16_t)length);
}

/**
  * @brief  Write configuration to specific bank
  * @param  address: Bank address to write to
  * @param  config: Pointer to configuration to write
  * @param  sequence: Sequence number to assign
  * @retval NVM operation result
  */
static nvm_result_t NVM_WriteDataToBank(uint32_t address, const uint8_t* data, uint32_t data_size, uint32_t sequence)
{
    nvm_data_storage_t storage;
    
    /* Fill storage structure */
    storage.magic = CONFIG_MAGIC_NUMBER;
    storage.version = CONFIG_VERSION;
    storage.sequence = sequence;
    storage.data_size = data_size;
    
    /* Copy data */
    NVM_MemCpy(storage.data, data, data_size);
    
    /* Calculate and store CRC */
    storage.crc16 = NVM_CalculateCRC16(storage.data, storage.data_size);
    storage.padding = 0; /* Ensure padding is zero */
    
    /* Write to flash */
    return NVM_WriteFlash(address, (const uint8_t*)&storage, sizeof(storage));
}

/**
  * @brief  Read data from specific bank (private function)
  * @param  address: Bank address to read from
  * @param  data: Pointer to buffer to fill with data
  * @param  max_size: Maximum size of the buffer
  * @param  actual_size: Pointer to receive actual data size read
  * @retval NVM operation result
  */
static nvm_result_t NVM_ReadDataFromBank(uint32_t address, uint8_t* data, uint32_t max_size, uint32_t* actual_size)
{
    nvm_data_storage_t storage;
    nvm_result_t result;
    
    /* Read storage structure from flash */
    result = NVM_ReadFlash(address, (uint8_t*)&storage, sizeof(storage));
    if (result != NVM_OK)
    {
        return result;
    }
    
    /* Validate storage */
    result = NVM_ValidateStorage(&storage);
    if (result != NVM_OK)
    {
        return result;
    }
    
    /* Check if data fits in provided buffer */
    if (storage.data_size > max_size)
    {
        return NVM_INVALID_PARAM;
    }
    
    /* Copy data */
    NVM_MemCpy(data, storage.data, storage.data_size);
    *actual_size = storage.data_size;
    
    return NVM_OK;
}

/**
  * @brief  Simple memory copy function
  * @param  dest: Destination buffer
  * @param  src: Source buffer
  * @param  length: Number of bytes to copy
  * @retval None
  */
static void NVM_MemCpy(uint8_t* dest, const uint8_t* src, uint32_t length)
{
    if (dest == NULL || src == NULL || length == 0)
    {
        return;
    }
    
    for (uint32_t i = 0; i < length; i++)
    {
        dest[i] = src[i];
    }
}