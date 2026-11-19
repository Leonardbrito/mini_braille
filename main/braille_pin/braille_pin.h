/**
 * @file braille_pin.h
 * @brief Public API of a single braille dot
 *
 * Knows the state of the dot (raised or lowered) and how many motor steps separate
 * the two stable positions of the mechanism; knows nothing about GPIOs or phases.
 * A 2x3 braille cell will be a set of six of these.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "stepper.h"

/** @brief Braille dot instance, filled by braille_pin_init() */
typedef struct
{
    stepper_t *motor;  /**< Motor that moves this dot, not owned by it */
    int stroke_steps;  /**< Motor steps between the lowered and raised positions */
    bool raised;       /**< Current state of the dot */
    uint8_t raised_hold_duty_pct;  /**< Coil PWM duty held while raised, 0 releases the coils */
} braille_pin_t;

/**
 * @brief Initializes a braille dot
 *
 * There is no position sensor: the initial state is only what the caller assumes.
 *
 * @param[out] pin Instance to initialize
 * @param motor Initialized motor that moves the dot
 * @param stroke_steps Motor steps between the lowered and raised positions, must be positive
 * @param raised Assumed initial state, true if the dot starts raised
 * @param raised_hold_duty_pct PWM duty (0-100) the coils keep while the dot is raised, so it
 *        resists being pushed down; 0 releases them. The dot is always released when lowered.
 * @return uint8_t
 *   - 0: success
 *   - 1: invalid argument
 */
uint8_t braille_pin_init(braille_pin_t *pin, stepper_t *motor, int stroke_steps, bool raised,
                         uint8_t raised_hold_duty_pct);

/**
 * @brief Gets the current state of the dot
 *
 * @param pin Initialized instance
 * @return true if the dot is raised
 * @return false if the dot is lowered
 */
bool braille_pin_is_raised(const braille_pin_t *pin);

/**
 * @brief Raises the dot, doing nothing if it is already raised
 *
 * Blocks the calling task while the motor moves.
 *
 * @param pin Initialized instance
 */
void braille_pin_raise(braille_pin_t *pin);

/**
 * @brief Raises the dot a full stroke even if it is believed to be raised already
 *
 * For a dot that is pushed down by hand: the state is not trusted, the motor always moves.
 * If the dot is really still at the top, the motor pushes against the end of the stroke.
 * Blocks the calling task while the motor moves.
 *
 * @param pin Initialized instance
 */
void braille_pin_force_raise(braille_pin_t *pin);

/**
 * @brief Lowers the dot, doing nothing if it is already lowered
 *
 * Blocks the calling task while the motor moves.
 *
 * @param pin Initialized instance
 */
void braille_pin_lower(braille_pin_t *pin);

/**
 * @brief Puts the dot in the requested state
 *
 * @param pin Initialized instance
 * @param raised true to raise, false to lower
 */
void braille_pin_set(braille_pin_t *pin, bool raised);
