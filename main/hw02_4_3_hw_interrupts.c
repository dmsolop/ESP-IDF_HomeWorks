#include "sdkconfig.h"
#if CONFIG_HW_02_4_3_HW_INTERRUPTS
#include "hw02_4_3_hw_interrupts.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BUTTON_PIN 4

static volatile bool button_event_3 = false;
static bool is_btn_high = true;

static void IRAM_ATTR button_isr_handler_3(void *arg)
{
    button_event_3 = true;
}

void hw02_4_3_run(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_ANYEDGE};
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler_3, NULL);

    int valid_press_count = 0;

    while (1)
    {
        if (button_event_3)
        {
            button_event_3 = false;
            vTaskDelay(pdMS_TO_TICKS(20));

            if (gpio_get_level(BUTTON_PIN) == 0 && is_btn_high)
            {
                is_btn_high = false;
                valid_press_count++;
                printf("HW_02_4_3: Button pressed (State Check)! Count: %d\n", valid_press_count);
            }
            if (gpio_get_level(BUTTON_PIN) == 1)
            {
                is_btn_high = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#endif