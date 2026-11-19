#include "esp_log.h"

#include "braille_pin.h"

/** @brief Module log tag */
static const char *TAG = "braille_pin";

uint8_t braille_pin_init(braille_pin_t *pin, stepper_t *motor, int stroke_steps, bool raised)
{
    if (pin == NULL || motor == NULL || stroke_steps <= 0)
    {
        ESP_LOGE(TAG, "Invalid argument");
        return 1;
    }

    pin->motor = motor;
    pin->stroke_steps = stroke_steps;
    pin->raised = raised;
    return 0;
}

bool braille_pin_is_raised(const braille_pin_t *pin)
{
    return pin->raised;
}

void braille_pin_raise(braille_pin_t *pin)
{
    if (pin->raised)
    {
        return;
    }

    ESP_LOGI(TAG, "Raising (%d steps)", pin->stroke_steps);
    stepper_move(pin->motor, pin->stroke_steps);
    pin->raised = true;
}

void braille_pin_force_raise(braille_pin_t *pin)
{
    ESP_LOGI(TAG, "Forcing raise (%d steps)", pin->stroke_steps);
    stepper_move(pin->motor, pin->stroke_steps);
    pin->raised = true;
}

void braille_pin_step(braille_pin_t *pin, int steps)
{
    ESP_LOGI(TAG, "Stepping (%d steps)", steps);
    stepper_move(pin->motor, steps);
}

void braille_pin_lower(braille_pin_t *pin)
{
    if (!pin->raised)
    {
        return;
    }

    ESP_LOGI(TAG, "Lowering (%d steps)", pin->stroke_steps);
    stepper_move(pin->motor, -pin->stroke_steps);
    pin->raised = false;
}

void braille_pin_set(braille_pin_t *pin, bool raised)
{
    if (raised)
    {
        braille_pin_raise(pin);
    }
    else
    {
        braille_pin_lower(pin);
    }
}
