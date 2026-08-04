#include "hw02_4_4_hw_interrupts.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BUTTON_PIN 4

typedef enum
{
    BTN_IDLE,
    BTN_DEBOUNCE_PRESS,
    BTN_PRESSED,
    BTN_DEBOUNCE_RELEASE
} button_state_t;

void hw02_4_4_run(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    button_state_t state = BTN_IDLE;
    int valid_press_count = 0;
    uint8_t stability_counter = 0;
    const uint8_t STABILITY_THRESHOLD = 3;

    while (1)
    {
        int level = gpio_get_level(BUTTON_PIN);

        switch (state)
        {
        case BTN_IDLE:
            if (level == 0)
            {
                state = BTN_DEBOUNCE_PRESS;
                stability_counter = 0;
            }
            break;

        case BTN_DEBOUNCE_PRESS:
            if (level == 0)
            {
                stability_counter++;
                if (stability_counter >= STABILITY_THRESHOLD)
                {
                    state = BTN_PRESSED;
                    valid_press_count++;
                    printf("HW_02_4_4: Button pressed (Polling)! Count: %d\n", valid_press_count);
                }
            }
            else
            {
                state = BTN_IDLE; // Хибне спрацювання (брязкіт)
            }
            break;

        case BTN_PRESSED:
            if (level == 1)
            {
                state = BTN_DEBOUNCE_RELEASE;
                stability_counter = 0;
            }
            break;

        case BTN_DEBOUNCE_RELEASE:
            if (level == 1)
            {
                stability_counter++;
                if (stability_counter >= STABILITY_THRESHOLD)
                {
                    state = BTN_IDLE;
                }
            }
            else
            {
                state = BTN_PRESSED; // Брязкіт при відпусканні
            }
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}