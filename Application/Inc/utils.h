/**
  ******************************************************************************
  * @file           : utils.h
  * @brief          : Utility functions
  ******************************************************************************
  */

#ifndef __UTILS_H
#define __UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Function declarations ------------------------------------------------------*/
uint8_t utils_is_member(uint8_t value, const uint8_t* array, uint8_t length);
void utils_memcpy(uint8_t* dest, const uint8_t* src, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* __UTILS_H */
