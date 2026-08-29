#include "sdkconfig.h"

#if CONFIG_CW_SERVO_POT

#include "cw_servo_pot.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "CW_SERVO_POT";

// Константи параметрів ШІМ для сервопривода
#define SERVO_MODE LEDC_LOW_SPEED_MODE
#define SERVO_CHANNEL LEDC_CHANNEL_0
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_FREQ_HZ 50                   // 50 Гц (період 20 мс)
#define SERVO_RESOLUTION LEDC_TIMER_13_BIT // 13 біт = 8192 рівні
#define PERIOD_US 20000                    // 20 000 мкс = 20 мс

// Допоміжна функція лінійного масштабування (аналог map() з Arduino)
static long map_value(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void cw_servo_pot_run(void)
{
    // 1. Конфігурація апаратного таймера LEDC
    ledc_timer_config_t ledc_timer = {
        .speed_mode = SERVO_MODE,
        .duty_resolution = SERVO_RESOLUTION,
        .timer_num = SERVO_TIMER,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. Конфігурація каналу LEDC
    ledc_channel_config_t ledc_channel = {
        .speed_mode = SERVO_MODE,
        .channel = SERVO_CHANNEL,
        .timer_sel = SERVO_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = CONFIG_CW_SERVO_GPIO,
        .duty = 0,
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // 3. Конфігурація модуля АЦП (Oneshot)
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, CONFIG_CW_POT_ADC_CHANNEL, &config));

    ESP_LOGI(TAG, "Класну роботу ініціалізовано. Управління сервоприводом запущено.");

    // 4. Основний цикл виконання
    while (1)
    {
        int raw_adc_val = 0;

        if (adc_oneshot_read(adc_handle, CONFIG_CW_POT_ADC_CHANNEL, &raw_adc_val) == ESP_OK)
        {

            // Перетворення відліку АЦП (0..4095) у кут (0..180 град)
            long angle = map_value(raw_adc_val, 0, 4095, 0, 180);

            // Перетворення кута у тривалість позитивного імпульсу (мкс)
            long pulse_us = map_value(angle, 0, 180, CONFIG_CW_SERVO_MIN_PULSE_US, CONFIG_CW_SERVO_MAX_PULSE_US);

            // Перетворення тривалості імпульсу в значення для регістра Duty (0..8191)
            uint32_t duty = (pulse_us * (1 << SERVO_RESOLUTION)) / PERIOD_US;

            // Запис та застосування нового значення Duty
            ledc_set_duty(SERVO_MODE, SERVO_CHANNEL, duty);
            ledc_update_duty(SERVO_MODE, SERVO_CHANNEL);

            ESP_LOGI(TAG, "RAW ADC: %4d | Кут: %3ld° | Імпульс: %4ld us | Duty: %4lu",
                     raw_adc_val, angle, pulse_us, (unsigned long)duty);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

#endif // CONFIG_CW_SERVO_POT