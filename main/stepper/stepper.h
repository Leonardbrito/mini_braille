/**
 * @file stepper.h
 * @brief Public API of the bipolar stepper motor driver
 *
 * Drives a 2-phase motor through a dual H-bridge (two GPIOs per coil). Full-step
 * sequence with both coils energized: {A+,B+} -> {A-,B+} -> {A-,B-} -> {A+,B-};
 * running it backwards reverses the direction. Each GPIO is a PWM output, so the coils can
 * be driven at full current to move and at a reduced duty to hold the rotor.
 */

#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "driver/ledc.h"

/** @brief Stepper motor wiring and timing */
typedef struct
{
    gpio_num_t coil_a_in1;    /**< H-bridge input 1 of coil A */
    gpio_num_t coil_a_in2;    /**< H-bridge input 2 of coil A */
    gpio_num_t coil_b_in1;    /**< H-bridge input 1 of coil B */
    gpio_num_t coil_b_in2;    /**< H-bridge input 2 of coil B */
    uint32_t step_period_us;  /**< Time between steps in microseconds */
    ledc_channel_t first_channel;  /**< First of 4 consecutive LEDC channels used by the coils */
} stepper_config_t;

/** @brief Stepper motor instance, filled by stepper_init() */
typedef struct
{
    stepper_config_t config;  /**< Copy of the configuration given to stepper_init() */
    int phase;                /**< Current index in the step sequence (0..3) */
} stepper_t;

/**
 * @brief Initializes a stepper motor instance
 *
 * Configures the four GPIOs as PWM outputs and leaves both coils released.
 *
 * @param[out] motor Instance to initialize
 * @param[in] config Wiring and timing, copied into the instance
 * @return uint8_t
 *   - 0: success
 *   - 1: invalid argument or PWM configuration failure
 */
uint8_t stepper_init(stepper_t *motor, const stepper_config_t *config);

/**
 * @brief Moves the motor a number of steps, then holds or releases the coils
 *
 * Blocks the calling task for the whole movement. The coils run at full current while
 * moving.
 *
 * @param motor Initialized instance
 * @param steps Number of steps; the sign selects the direction, 0 does nothing
 * @param hold_duty_pct PWM duty in percent (0-100) kept on the coils at the last phase after
 *        the movement; 0 releases the coils, so the mechanism must hold the position by itself
 * @note A held coil heats the motor: the average current, and so the holding torque, scales
 *       with the duty, while the heat scales with its square. Keep it as low as the job allows.
 */
void stepper_move(stepper_t *motor, int steps, uint8_t hold_duty_pct);

/**
 * @brief Releases both coils (H-bridge outputs floating, no current)
 *
 * @param motor Initialized instance
 */
void stepper_release(stepper_t *motor);
