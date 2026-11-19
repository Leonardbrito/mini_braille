#include "esp_log.h"

#include "button.h"

/** @brief Consecutive polls with the same reading before the state is accepted (3 x 10 ms = 30 ms) */
#define BUTTON_DEBOUNCE_POLLS 3

/** @brief Module log tag */
static const char *TAG = "button";

uint8_t button_init(button_t *btn, gpio_num_t gpio)
{
    if (btn == NULL)
    {
        ESP_LOGE(TAG, "Invalid argument");
        return 1;
    }

    gpio_config_t io_config = {
        .pin_bit_mask = 1ULL << gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&io_config) != ESP_OK)
    {
        ESP_LOGE(TAG, "GPIO configuration failed");
        return 1;
    }

    btn->gpio = gpio;
    btn->stable_pressed = false;
    btn->raw_pressed = false;
    btn->raw_count = 0;
    return 0;
}

bool button_was_pressed(button_t *btn)
{
    bool raw_pressed = gpio_get_level(btn->gpio) == 0;

    if (raw_pressed != btn->raw_pressed)
    {
        btn->raw_pressed = raw_pressed;
        btn->raw_count = 0;
    }

    if (btn->raw_count < BUTTON_DEBOUNCE_POLLS)
    {
        btn->raw_count++;
    }

    if (btn->raw_count == BUTTON_DEBOUNCE_POLLS && btn->stable_pressed != btn->raw_pressed)
    {
        btn->stable_pressed = btn->raw_pressed;
        return btn->stable_pressed;
    }

    return false;
}
