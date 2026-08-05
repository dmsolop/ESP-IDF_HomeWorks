#include "sdkconfig.h"

#if CONFIG_HW_02_3_SUPERLOOP

#include "hw02_3_superloop.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static uint32_t millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

typedef struct
{
    gpio_num_t pin;
    uint32_t interval_ms;
    uint32_t last_toggle;
    bool state;
} led_config_t;

void hw02_3_superloop_run(void)
{
    printf("Starting HW02: Superloop 3 LEDs...\n");

    // Вкажи тут свої номери GPIO для трьох діодів
    led_config_t leds[] = {
        {.pin = GPIO_NUM_4, .interval_ms = 200, .last_toggle = 0, .state = false},
        {.pin = GPIO_NUM_5, .interval_ms = 500, .last_toggle = 0, .state = false},
        {.pin = GPIO_NUM_6, .interval_ms = 1000, .last_toggle = 0, .state = false}};

    size_t num_leds = sizeof(leds) / sizeof(leds[0]);

    for (size_t i = 0; i < num_leds; i++)
    {
        gpio_reset_pin(leds[i].pin);
        gpio_set_direction(leds[i].pin, GPIO_MODE_OUTPUT);
        gpio_set_level(leds[i].pin, leds[i].state);
    }

    while (1)
    {
        uint32_t current_time = millis();

        for (size_t i = 0; i < num_leds; i++)
        {
            if (current_time - leds[i].last_toggle >= leds[i].interval_ms)
            {
                leds[i].last_toggle = current_time;
                leds[i].state = !leds[i].state;
                gpio_set_level(leds[i].pin, leds[i].state);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#endif