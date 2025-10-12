/**
  ******************************************************************************
  * @file           : utils.c
  * @brief          : Utility functions implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "utils.h"

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
