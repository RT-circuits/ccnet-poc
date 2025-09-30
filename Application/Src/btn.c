/**
  ******************************************************************************
  * @file           : btn.c
  * @brief          : Button handling implementation
  *                   Config/Reset button functionality
  ******************************************************************************
  * @attention
  *
  * Copyright (c) pdewit.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "btn.h"
#include "led.h"
#include "config.h"
#include "app.h"

/* Private variables ---------------------------------------------------------*/
static config_reset_button_t cr_button = {0};
static uint8_t config_menu_active = 0;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize button handling
  * @retval None
  */
void BTN_Init(void)
{
    // Initialize button state
    cr_button.press_detected = 0;
    cr_button.press_start_time = 0;
    cr_button.long_press_detected = 0;
    cr_button.just_pressed = 0;
    cr_button.just_released = 0;
    
    config_menu_active = 0;
}

/**
  * @brief  Config/Reset button interrupt handler (both rising and falling edge)
  * @note   Short press: Config menu, Long press: MCU reset
  * @retval None
  */
void BTN_ConfigResetButtonInterrupt(void)
{
    // Clear the interrupt flag first
    __HAL_GPIO_EXTI_CLEAR_IT(CONFIG_RESET_BUTTON_PIN);
    
    // Check current button state
    if (HAL_GPIO_ReadPin(CONFIG_RESET_BUTTON_PORT, CONFIG_RESET_BUTTON_PIN) == GPIO_PIN_SET)
    {
        // Rising edge - button pressed
        cr_button.press_detected = 1;
        cr_button.press_start_time = HAL_GetTick();
        cr_button.long_press_detected = 0;
        cr_button.just_pressed = 1;
    }
    else
    {
        // Falling edge - button released
        if (cr_button.press_detected)
        {
            // Mark button as released - LED actions will be handled in main loop
            cr_button.press_detected = 0;
            cr_button.just_released = 1;
        }
    }
}

/**
  * @brief  Process config/reset button state and handle LED actions
  * @retval None
  */
void BTN_ProcessConfigResetButton(void)
{
    /* Handle button press events */
    if (cr_button.just_pressed)
    {
        // Button just pressed - turn on LED3 (indicates button is pressed)
        LED_On(&hled3);
        cr_button.just_pressed = 0;
    }
    
    /* Handle button release events */
    if (cr_button.just_released)
    {
        // Button just released - handle LED actions and functionality based on press duration
        if (!cr_button.long_press_detected)
        {
            // Short press - flash LED1 (config menu indicator)
            LED_Flash(&hled1, CONFIG_RESET_LED_FLASH_MS);
            
            
            // Start config menu
            CONFIG_DisplayCurrentSettings();
            CONFIG_ShowMenu();
            config_menu_active = 1;
        }
        else
        {
            // Long press - flash LED2 (reset indicator)
            LED_Flash(&hled2, CONFIG_RESET_LED_FLASH_MS);
            
            // Restart MCU
            CONFIG_BufferWrite("\r\n=== LONG PRESS DETECTED - RESTARTING MCU ===\r\n");
            CONFIG_FlushBuffer();
            HAL_Delay(100); // Give time for message to send
            APP_MCUReset();
        }
        cr_button.just_released = 0;
    }
    
    /* Check for long press detection (polling approach) */
    if (cr_button.press_detected)
    {
        // Check if button is still pressed by reading GPIO
        if (HAL_GPIO_ReadPin(CONFIG_RESET_BUTTON_PORT, CONFIG_RESET_BUTTON_PIN) == GPIO_PIN_SET)
        {
            // Button still pressed - check for long press threshold
            uint32_t press_duration = HAL_GetTick() - cr_button.press_start_time;
            if (press_duration >= CONFIG_RESET_LONG_PRESS_MS && !cr_button.long_press_detected)
            {
                cr_button.long_press_detected = 1;
                // Turn off LED3 to indicate long press detected
                LED_Off(&hled3);
            }
        }
    }
}

/**
  * @brief  Check if config menu is active
  * @retval 1 if config menu is active, 0 otherwise
  */
uint8_t BTN_IsConfigMenuActive(void)
{
    return config_menu_active;
}
