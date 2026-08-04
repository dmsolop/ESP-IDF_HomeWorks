#include "hw02_4_2_hw_interrupts.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define BUTTON_PIN 4
#define DEBOUNCE_TIME_US 50000

static volatile bool button_event_2 = false;

static void IRAM_ATTR button_isr_handler_2(void *arg)
{
    button_event_2 = true;
}

void hw02_4_2_run(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_NEGEDGE};
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler_2, NULL);

    int valid_press_count = 0;
    uint64_t last_event_time = 0;

    while (1)
    {
        if (button_event_2)
        {
            button_event_2 = false;
            uint64_t current_time = esp_timer_get_time();

            if ((current_time - last_event_time) > DEBOUNCE_TIME_US)
            {
                valid_press_count++;
                printf("HW_02_4_2: Button pressed (Time Debounce)! Count: %d\n", valid_press_count);
                last_event_time = current_time;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}