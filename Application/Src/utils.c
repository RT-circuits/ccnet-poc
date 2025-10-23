/**
  ******************************************************************************
  * @file           : utils.c
  * @brief          : Utility functions implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "utils.h"
#include "main.h"

/*********************************************
 * Array functions
 *********************************************/
/* uint8 membership check */
uint8_t utils_is_member(uint8_t value, const uint8_t* array, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) 
        if (array[i] == value) return 1;
    return 0;
}

/* memcpy */
void utils_memcpy(uint8_t* dest, const uint8_t* src, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) dest[i] = src[i];
}

/* memset to zero */
void utils_zero(uint8_t* array, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) array[i] = 0;
}
/***********************************************************************     
 * String functions
 * to allow all modules to run without snprintf if logging is disabled
 ***********************************************************************/

/* Convert uint32_t to decimal string */
void utils_uint32_to_string(uint32_t value, char* buffer, uint32_t buffer_size) {
    if (buffer == NULL || buffer_size == 0) return;
    
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    
    char temp[12]; // Max digits for uint32_t
    uint32_t pos = 0;
    uint32_t temp_value = value;
    
    // Convert to string in reverse order
    while (temp_value > 0) {
        temp[pos++] = '0' + (temp_value % 10);
        temp_value /= 10;
    }
    
    // Reverse the string and copy to buffer
    uint32_t i = 0;
    while (pos > 0 && i < buffer_size - 1) {
        buffer[i++] = temp[--pos];
    }
    buffer[i] = '\0';
}

/* Concatenate string and uint32_t */
void utils_string_uint32_concat(const char* str, uint32_t value, char* buffer, uint32_t buffer_size) {
    if (buffer == NULL || buffer_size == 0) return;
    
    uint32_t pos = 0;
    
    // Find end of string
    if (str != NULL) {
        while (*str != '\0' && pos < buffer_size - 1) {
            buffer[pos++] = *str++;
        }
    }
    
    // Convert uint32_t to string using existing function
    char value_str[16];
    utils_uint32_to_string(value, value_str, sizeof(value_str));
    
    // Append the converted value string
    uint32_t i = 0;
    while (value_str[i] != '\0' && pos < buffer_size - 1) {
        buffer[pos++] = value_str[i++];
    }
    
    buffer[pos] = '\0';
}
