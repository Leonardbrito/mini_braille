#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_rom_sys.h"

#include "stepper.h"

/** @brief Pause after the last step, before the coils are held or released, in milliseconds */
#define STEPPER_SETTLE_MS 10

/** @brief PWM duty while moving, in percent */
#define STEPPER_MOVE_DUTY_PCT 100

/**
 * @brief PWM frequency in hertz
 *
 * Above the audible range, otherwise the held coils whine. The coil inductance smooths the
 * current at this frequency.
 */
#define STEPPER_PWM_FREQ_HZ 20000

/** @brief PWM resolution: 10 bits fit 20 kHz on the 80 MHz LEDC clock of the classic ESP32 */
#define STEPPER_PWM_RESOLUTION LEDC_TIMER_10_BIT

/** @brief Highest duty value at the PWM resolution (100%) */
#define STEPPER_PWM_MAX_DUTY ((1 << STEPPER_PWM_RESOLUTION) - 1)

/** @brief LEDC timer shared by every motor */
#define STEPPER_PWM_TIMER LEDC_TIMER_0

/** @brief LEDC speed mode: the only one available on every ESP32 variant */
#define STEPPER_PWM_SPEED_MODE LEDC_LOW_SPEED_MODE

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
 * @param duty_pct PWM duty in percent (0-100); 0 releases both coils
 */
static void apply_phase(const stepper_t *motor, uint8_t duty_pct);

/**
 * @brief Sets the current through one coil
 *
 * @param motor Initialized instance
 * @param coil Coil index: 0 for A, 1 for B
 * @param direction +1 and -1 select the current direction, 0 releases the coil
 * @param duty_pct PWM duty in percent (0-100) of the energized H-bridge input
 */
static void set_coil(const stepper_t *motor, int coil, int8_t direction, uint8_t duty_pct);

/**
 * @brief Sets the PWM duty of one H-bridge input
 *
 * @param motor Initialized instance
 * @param input Input index from the first channel: 0 A-IN1, 1 A-IN2, 2 B-IN1, 3 B-IN2
 * @param duty_pct PWM duty in percent (0-100)
 */
static void set_input_duty(const stepper_t *motor, int input, uint8_t duty_pct);

uint8_t stepper_init(stepper_t *motor, const stepper_config_t *config)
{
    if (motor == NULL || config == NULL)
    {
        ESP_LOGE(TAG, "Invalid argument");
        return 1;
    }

    motor->config = *config;
    motor->phase = 0;

    const ledc_timer_config_t timer_config = {
        .speed_mode = STEPPER_PWM_SPEED_MODE,
        .timer_num = STEPPER_PWM_TIMER,
        .duty_resolution = STEPPER_PWM_RESOLUTION,
        .freq_hz = STEPPER_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    if (ledc_timer_config(&timer_config) != ESP_OK)
    {
        ESP_LOGE(TAG, "PWM timer configuration failed");
        return 1;
    }

    const gpio_num_t inputs[4] = {config->coil_a_in1, config->coil_a_in2, config->coil_b_in1,
                                  config->coil_b_in2};

    for (int i = 0; i < 4; i++)
    {
        const ledc_channel_config_t channel_config = {
            .gpio_num = inputs[i],
            .speed_mode = STEPPER_PWM_SPEED_MODE,
            .channel = config->first_channel + i,
            .timer_sel = STEPPER_PWM_TIMER,
            .duty = 0,
            .hpoint = 0,
        };

        if (ledc_channel_config(&channel_config) != ESP_OK)
        {
            ESP_LOGE(TAG, "PWM channel %d configuration failed", i);
            return 1;
        }
    }

    stepper_release(motor);
    return 0;
}

void stepper_move(stepper_t *motor, int steps, uint8_t hold_duty_pct)
{
    if (steps == 0)
    {
        return;
    }

    int direction = steps > 0 ? 1 : -1;

    /* the FreeRTOS tick (10 ms by default) is far too coarse for steps of ~2 ms */
    apply_phase(motor, STEPPER_MOVE_DUTY_PCT);
    esp_rom_delay_us(motor->config.step_period_us);

    for (int i = 0; i < abs(steps); i++)
    {
        motor->phase = (motor->phase + direction + 4) % 4;
        apply_phase(motor, STEPPER_MOVE_DUTY_PCT);
        esp_rom_delay_us(motor->config.step_period_us);
    }

    /* lets the rotor settle on the last phase before the coils are held or released */
    vTaskDelay(pdMS_TO_TICKS(STEPPER_SETTLE_MS));

    /* a duty of 0 leaves both inputs low, which is the same as releasing the coils */
    apply_phase(motor, hold_duty_pct);
}

void stepper_release(stepper_t *motor)
{
    apply_phase(motor, 0);
}

static void apply_phase(const stepper_t *motor, uint8_t duty_pct)
{
    set_coil(motor, 0, step_sequence[motor->phase][0], duty_pct);
    set_coil(motor, 1, step_sequence[motor->phase][1], duty_pct);
}

static void set_coil(const stepper_t *motor, int coil, int8_t direction, uint8_t duty_pct)
{
    set_input_duty(motor, coil * 2, direction > 0 ? duty_pct : 0);
    set_input_duty(motor, coil * 2 + 1, direction < 0 ? duty_pct : 0);
}

static void set_input_duty(const stepper_t *motor, int input, uint8_t duty_pct)
{
    if (duty_pct > 100)
    {
        duty_pct = 100;
    }

    ledc_channel_t channel = motor->config.first_channel + input;

    ledc_set_duty(STEPPER_PWM_SPEED_MODE, channel, duty_pct * STEPPER_PWM_MAX_DUTY / 100);
    ledc_update_duty(STEPPER_PWM_SPEED_MODE, channel);
}
