/**
 * @file button.h
 * @brief Public API of a debounced push button
 *
 * The button connects the GPIO to GND (active low); the internal pull-up keeps the
 * line high while it is released. The caller polls button_was_pressed() periodically.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"

/** @brief Push button instance, filled by button_init() */
typedef struct
{
    gpio_num_t gpio;      /**< GPIO connected to the button */
    bool stable_pressed;  /**< Debounced state of the button */
    bool raw_pressed;     /**< Last raw reading, a candidate for the new stable state */
    uint32_t raw_count;   /**< Consecutive polls that returned the raw reading */
} button_t;

/**
 * @brief Initializes a push button instance
 *
 * Configures the GPIO as an input with the internal pull-up enabled.
 *
 * @param[out] btn Instance to initialize
 * @param gpio GPIO connected to the button (other terminal goes to GND)
 * @return uint8_t
 *   - 0: success
 *   - 1: invalid argument or GPIO configuration failure
 */
uint8_t button_init(button_t *btn, gpio_num_t gpio);

/**
 * @brief Polls the button and reports a new press
 *
 * Must be called every BUTTON_POLL_MS. A press is reported once, when the debounced state
 * changes from released to pressed; holding the button does not repeat it.
 *
 * @param btn Initialized instance
 * @return true if the button was just pressed
 * @return false otherwise
 */
bool button_was_pressed(button_t *btn);
