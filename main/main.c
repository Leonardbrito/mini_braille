#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "braille_pin.h"
#include "button.h"
#include "stepper.h"

/** @brief Module log tag */
static const char *TAG = "main";

/** @brief Motor under test */
static stepper_t motor;

/** @brief Braille dot driven by the motor, assumed lowered at boot (assembled at rest) */
static braille_pin_t pin;

/** @brief Button that toggles the dot between lowered and raised */
static button_t toggle_button;

/**
 * @brief Initializes every module, in dependency order
 *
 * @return uint8_t
 *   - 0: success
 *   - 1: initialization failure
 */
static uint8_t app_init(void);

void app_main(void)
{
    if (app_init())
    {
        return;
    }

    /* the dot is assembled at rest, and the app starts with it raised and held */
    braille_pin_raise(&pin);

    /* TEMPORARY diagnostic: remove once the button is confirmed working */
    int last_level = gpio_get_level(STEP_BUTTON_GPIO);
    ESP_LOGI(TAG, "DEBUG GPIO%d idle level = %d (expected 1)", STEP_BUTTON_GPIO, last_level);

    for (;;)
    {
        int level = gpio_get_level(STEP_BUTTON_GPIO);
        if (level != last_level)
        {
            last_level = level;
            ESP_LOGI(TAG, "DEBUG GPIO%d level changed to %d", STEP_BUTTON_GPIO, level);
        }

        if (button_was_pressed(&toggle_button))
        {
            /* the raised dot is held by the coils and the lowered one by the yoke dead point, so the
             * assumed state can be trusted */
            braille_pin_set(&pin, !braille_pin_is_raised(&pin));
        }

        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}

static uint8_t app_init(void)
{
    const stepper_config_t motor_config = {
        .coil_a_in1 = STEPPER_COIL_A_IN1_GPIO,
        .coil_a_in2 = STEPPER_COIL_A_IN2_GPIO,
        .coil_b_in1 = STEPPER_COIL_B_IN1_GPIO,
        .coil_b_in2 = STEPPER_COIL_B_IN2_GPIO,
        .step_period_us = STEPPER_STEP_PERIOD_US,
        .first_channel = STEPPER_LEDC_FIRST_CHANNEL,
    };

    if (stepper_init(&motor, &motor_config))
    {
        ESP_LOGE(TAG, "Failed to initialize stepper");
        return 1;
    }

    if (braille_pin_init(&pin, &motor, BRAILLE_PIN_STROKE_STEPS, false,
                         BRAILLE_PIN_RAISED_HOLD_DUTY_PCT))
    {
        ESP_LOGE(TAG, "Failed to initialize braille pin");
        return 1;
    }

    if (button_init(&toggle_button, STEP_BUTTON_GPIO))
    {
        ESP_LOGE(TAG, "Failed to initialize toggle button");
        return 1;
    }

    return 0;
}
