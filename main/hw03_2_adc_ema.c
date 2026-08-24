#include "sdkconfig.h"

#if CONFIG_HW_03_2_ADC_EMA

#include "hw03_2_adc_ema.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sensor_ldr.h"

static const char *TAG = "HW03_2";
#define LED_GPIO GPIO_NUM_4

void hw03_2_run(void)
{
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&led_conf);

    ESP_ERROR_CHECK(sensor_ldr_init());
    ESP_LOGI(TAG, "LDR Sensor initialized via traffic light component.");

    while (1)
    {
        ldr_mode_t mode = sensor_ldr_get_mode();
        int raw_val = sensor_ldr_read_raw();
        int ema_mv = sensor_ldr_get_filtered_mv();

        gpio_set_level(LED_GPIO, (mode == LDR_MODE_NIGHT) ? 1 : 0);

        ESP_LOGI(TAG, "RAW ADC: %4d | EMA: %4d mV | Mode: %s",
                 raw_val, ema_mv, (mode == LDR_MODE_NIGHT) ? "NIGHT" : "DAY");

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

#endif