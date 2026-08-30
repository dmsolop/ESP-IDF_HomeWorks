#include "sdkconfig.h"

#if CONFIG_HW_03_5_SERVO_CONTROL

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "hw03_5_servo.h"

static const char *TAG = "SERVO_CONTROL";

// Конфігурація ШІМ (LEDC) для сервоприводу
#define SERVO_LEDC_MODE LEDC_LOW_SPEED_MODE
#define SERVO_LEDC_TIMER LEDC_TIMER_0
#define SERVO_LEDC_CHANNEL LEDC_CHANNEL_0
#define SERVO_LEDC_RESOLUTION LEDC_TIMER_13_BIT // 8192 відліки
#define SERVO_LEDC_FREQ_HZ 50                   // 50 Гц = період 20 000 мкс

// Межі тривалості імпульсів сервоприводу (в мікросекундах)
#define SERVO_MIN_PULSE_US 500  // 0 градусів
#define SERVO_MAX_PULSE_US 2500 // 180 градусів
#define SERVO_MAX_ANGLE 180     // Робочий кут сервоприводу

// Розрахунок duty для 13 біт при 50 Гц (20 000 мкс)
// Duty = (Pulse_us / 20000_us) * 8192
#define SERVO_MIN_DUTY 205  // (500 / 20000) * 8192
#define SERVO_MAX_DUTY 1024 // (2500 / 20000) * 8192

// Конфігурація АЦП (ADC1)
#define POT_ADC_UNIT ADC_UNIT_1
#define POT_ADC_CHANNEL ADC_CHANNEL_7 // GPIO 8 на ESP32-C3 / S3

// Коефіцієнт згладжування EMA
#define EMA_ALPHA 0.15f

// Фізичний кут повороту потенціометра (зазвичай ~270 градусів)
#define POT_FULL_ANGLE 270

// Функція обмеження діапазону (Clamp)
static inline int clamp_int(int val, int min, int max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

void hw03_5_run(void)
{
    // 1. Налаштування АЦП
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = POT_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, // 12 біт (0..4095)
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, POT_ADC_CHANNEL, &chan_config));

    // 2. Налаштування LEDC Таймера
    ledc_timer_config_t timer_conf = {
        .speed_mode = SERVO_LEDC_MODE,
        .duty_resolution = SERVO_LEDC_RESOLUTION,
        .timer_num = SERVO_LEDC_TIMER,
        .freq_hz = SERVO_LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    // 3. Налаштування LEDC Каналу
    ledc_channel_config_t channel_conf = {
        .speed_mode = SERVO_LEDC_MODE,
        .channel = SERVO_LEDC_CHANNEL,
        .timer_sel = SERVO_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = CONFIG_HW_03_5_SERVO_GPIO,
        .duty = SERVO_MIN_DUTY,
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    ESP_LOGI(TAG, "Керування сервоприводом запущено на GPIO %d", CONFIG_HW_03_5_SERVO_GPIO);

    float ema_filtered_adc = 0.0f;
    int prev_angle = -1;

    // Розрахунок максимального відліку АЦП, який відповідає куту 180 градусів (з 270 можливих)
    // 4095 * (180 / 270) = 2730
    const int pot_adc_180_deg_limit = (4095 * SERVO_MAX_ANGLE) / POT_FULL_ANGLE;

    while (1)
    {
        int raw_adc = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, POT_ADC_CHANNEL, &raw_adc));

        // Фільтрація шуму через EMA
        ema_filtered_adc = (EMA_ALPHA * (float)raw_adc) + ((1.0f - EMA_ALPHA) * ema_filtered_adc);
        int current_adc = (int)ema_filtered_adc;

        // Обрізаємо АЦП до межі, що відповідає куту 180 градусів сервоприводу
        int clamped_adc = clamp_int(current_adc, 0, pot_adc_180_deg_limit);

        // Мапуємо АЦП у кут відхилення 0..180 градусів (пропорція 1:1)
        int current_angle = (clamped_adc * SERVO_MAX_ANGLE) / pot_adc_180_deg_limit;

        // Перетворюємо кут у значення duty для ШІМ
        uint32_t duty = SERVO_MIN_DUTY +
                        ((current_angle * (SERVO_MAX_DUTY - SERVO_MIN_DUTY)) / SERVO_MAX_ANGLE);

        // Оновлюємо ШІМ лише якщо кут змінився
        if (current_angle != prev_angle)
        {
            ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, duty);
            ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);

            ESP_LOGI(TAG, "Кут відхилення: %d deg (ADC: %d, Duty: %lu)",
                     current_angle, raw_adc, duty);

            prev_angle = current_angle;
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // Оновлення кожні 20 мс
    }
}

#endif