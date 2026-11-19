#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_rom_sys.h"

#include "stepper.h"

/** @brief Pause after the last step, before the coils are released, in milliseconds */
#define STEPPER_SETTLE_MS 10

/** @brief Module log tag */
static const char *TAG = "stepper";

/**
 * @brief Current direction of each coil in each phase of the full-step sequence
 *
 * +1 and -1 are the two current directions of an H-bridge channel.
 */
static const int8_t step_sequence[4][2] = {{+1, +1}, {-1, +1}, {-1, -1}, {+1, -1}};

/**
 * @brief Energizes both coils according to the current phase
 *
 * @param motor Initialized instance
 */
static void apply_phase(const stepper_t *motor);

/**
 * @brief Sets the current through one coil
 *
 * @param in1 H-bridge input 1 of the coil
 * @param in2 H-bridge input 2 of the coil
 * @param direction +1 and -1 select the current direction, 0 releases the coil
 */
static void set_coil(gpio_num_t in1, gpio_num_t in2, int8_t direction);

uint8_t stepper_init(stepper_t *motor, const stepper_config_t *config)
{
    if (motor == NULL || config == NULL)
    {
        ESP_LOGE(TAG, "Invalid argument");
        return 1;
    }

    motor->config = *config;
    motor->phase = 0;

    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << config->coil_a_in1) | (1ULL << config->coil_a_in2) |
                        (1ULL << config->coil_b_in1) | (1ULL << config->coil_b_in2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&io_config) != ESP_OK)
    {
        ESP_LOGE(TAG, "GPIO configuration failed");
        return 1;
    }

    stepper_release(motor);
    return 0;
}

void stepper_move(stepper_t *motor, int steps)
{
    if (steps == 0)
    {
        return;
    }

    int direction = steps > 0 ? 1 : -1;

    /* the FreeRTOS tick (10 ms by default) is far too coarse for steps of ~2 ms */
    apply_phase(motor);
    esp_rom_delay_us(motor->config.step_period_us);

    for (int i = 0; i < abs(steps); i++)
    {
        motor->phase = (motor->phase + direction + 4) % 4;
        apply_phase(motor);
        esp_rom_delay_us(motor->config.step_period_us);
    }

    /* lets the rotor settle on the last phase before the coils are released */
    vTaskDelay(pdMS_TO_TICKS(STEPPER_SETTLE_MS));
    stepper_release(motor);
}

void stepper_release(stepper_t *motor)
{
    set_coil(motor->config.coil_a_in1, motor->config.coil_a_in2, 0);
    set_coil(motor->config.coil_b_in1, motor->config.coil_b_in2, 0);
}

static void apply_phase(const stepper_t *motor)
{
    set_coil(motor->config.coil_a_in1, motor->config.coil_a_in2, step_sequence[motor->phase][0]);
    set_coil(motor->config.coil_b_in1, motor->config.coil_b_in2, step_sequence[motor->phase][1]);
}

static void set_coil(gpio_num_t in1, gpio_num_t in2, int8_t direction)
{
    gpio_set_level(in1, direction > 0);
    gpio_set_level(in2, direction < 0);
}
