/**
 * @file board_config.h
 * @brief Pins and hardware constants of the board
 *
 * Force-included in every translation unit of main (see main/CMakeLists.txt).
 *
 * Wiring (1 motor, MX1508 H-bridge module sold as "L298N SMD"):
 *   GPIO15 -> IN1, GPIO4 -> IN2 (MOTOR-A, coil A)
 *   GPIO5 -> IN3, GPIO18 -> IN4 (MOTOR-B, coil B)
 *   3V3 -> "+", GND -> "-" (common ground is mandatory)
 *
 * GPIOs to avoid on the classic ESP32: 0, 2, 5, 12, 15 (strapping), 6-11 (flash),
 * 34-39 (input only).
 */

#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"

/** @brief H-bridge input 1 of coil A (module IN1) */
#define STEPPER_COIL_A_IN1_GPIO GPIO_NUM_15

/** @brief H-bridge input 2 of coil A (module IN2) */
#define STEPPER_COIL_A_IN2_GPIO GPIO_NUM_4

/** @brief H-bridge input 1 of coil B (module IN3) */
#define STEPPER_COIL_B_IN1_GPIO GPIO_NUM_5

/** @brief H-bridge input 2 of coil B (module IN4) */
#define STEPPER_COIL_B_IN2_GPIO GPIO_NUM_18

/** @brief First of the 4 consecutive LEDC channels that PWM the H-bridge inputs */
#define STEPPER_LEDC_FIRST_CHANNEL LEDC_CHANNEL_0

/**
 * @brief Time between motor steps in microseconds
 *
 * 2000 us = 500 steps/s. Increase if the motor loses steps; decrease for speed
 * (the SMS6-F40 datasheet tops out around 4900 steps/s; this motor's model is unconfirmed).
 */
#define STEPPER_STEP_PERIOD_US 20000

/**
 * @brief Motor steps between the lowered and raised positions of a pin
 *
 * The motor measured on the bench has 20 steps/turn (18 degrees/step), not the 40 of the
 * SMS6-F40 datasheet.
 * Eccentric cam: 10 steps = half a turn (confirmed on the bench).
 * M1x0.25 screw with 0.7 mm of travel: 56 steps (2.8 turns).
 */
#define BRAILLE_PIN_STROKE_STEPS 10

/**
 * @brief PWM duty in percent that holds the coils while the dot is raised
 *
 * Makes the dot resist being pushed down. Holding torque scales with the duty and heating
 * with its square (full current, 100%, was measured on the bench at ~330 mA and got hot).
 * Start low, check the motor temperature by touch and the resistance by pressing the dot.
 */
#define BRAILLE_PIN_RAISED_HOLD_DUTY_PCT 50

/**
 * @brief GPIO of the step button
 *
 * Button between this GPIO and GND; the internal pull-up keeps it high while released.
 * GPIO27 is not a strapping, flash or input-only pin.
 */
#define STEP_BUTTON_GPIO GPIO_NUM_13

/** @brief Button polling period in milliseconds (debounce window is 3 polls) */
#define BUTTON_POLL_MS 10
