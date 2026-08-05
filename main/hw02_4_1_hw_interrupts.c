#include "sdkconfig.h"
#if CONFIG_HW_02_4_1_HW_INTERRUPTS
#include "hw02_4_1_hw_interrupts.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BUTTON_PIN 4

static volatile int press_count = 0;

static void IRAM_ATTR button_isr_handler_1(void *arg)
{
    press_count++;
}

void hw02_4_1_run(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_NEGEDGE};
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler_1, NULL);

    int last_count = 0;

    while (1)
    {
        if (press_count != last_count)
        {
            last_count = press_count;
            printf("HW_02_4_1: Button pressed! Total counts: %d\n", last_count);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#endif