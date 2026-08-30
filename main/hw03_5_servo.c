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

// Фізичні кути компонентів (за замовчуванням для стандартних залізок)
#define SERVO_MAX_ANGLE 180 // Робочий кут сервоприводу в градусах
#define POT_FULL_ANGLE 270  // Повний механічний кут потенціометра в градусах

// Налаштування ШІМ (LEDC)
#define SERVO_LEDC_MODE LEDC_LOW_SPEED_MODE
#define SERVO_LEDC_TIMER LEDC_TIMER_0
#define SERVO_LEDC_CHANNEL LEDC_CHANNEL_0
#define SERVO_LEDC_RESOLUTION LEDC_TIMER_13_BIT
#define SERVO_LEDC_FREQ_HZ 50

#define US_PER_PERIOD 20000
#define LEDC_MAX_TICKS 8192

#define POT_ADC_UNIT ADC_UNIT_1
#define EMA_ALPHA 0.15f

// Перетворення мікросекунд у відліки duty
static inline uint32_t pulse_us_to_duty(uint32_t pulse_us)
{
    return (pulse_us * LEDC_MAX_TICKS) / US_PER_PERIOD;
}

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
    // Обчислення меж Duty з параметрів Kconfig
    const uint32_t min_duty = pulse_us_to_duty(CONFIG_HW_03_5_SERVO_MIN_PULSE_US);
    const uint32_t max_duty = pulse_us_to_duty(CONFIG_HW_03_5_SERVO_MAX_PULSE_US);

    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = POT_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, CONFIG_HW_03_5_POT_ADC_CHANNEL, &chan_config));

    ledc_timer_config_t timer_conf = {
        .speed_mode = SERVO_LEDC_MODE,
        .duty_resolution = SERVO_LEDC_RESOLUTION,
        .timer_num = SERVO_LEDC_TIMER,
        .freq_hz = SERVO_LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .speed_mode = SERVO_LEDC_MODE,
        .channel = SERVO_LEDC_CHANNEL,
        .timer_sel = SERVO_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = CONFIG_HW_03_5_SERVO_GPIO,
        .duty = min_duty,
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    ESP_LOGI(TAG, "Керування серво запущено. Серво GPIO: %d, ADC1 Канал: %d",
             CONFIG_HW_03_5_SERVO_GPIO, CONFIG_HW_03_5_POT_ADC_CHANNEL);

    float ema_filtered_adc = 0.0f;
    int prev_angle = -1;

    // Максимальне значення АЦП для пропорції 1:1 (4095 * 180 / 270 = 2730)
    const int pot_adc_limit = (4095 * SERVO_MAX_ANGLE) / POT_FULL_ANGLE;

    while (1)
    {
        int raw_adc = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, CONFIG_HW_03_5_POT_ADC_CHANNEL, &raw_adc));

        // Згладжування шуму АЦП
        ema_filtered_adc = (EMA_ALPHA * (float)raw_adc) + ((1.0f - EMA_ALPHA) * ema_filtered_adc);
        int current_adc = (int)ema_filtered_adc;

        // Обрізання АЦП під робочу зону 1:1
        int clamped_adc = clamp_int(current_adc, 0, pot_adc_limit);

        // Обчислення кута відхилення (0..180)
        int current_angle = (clamped_adc * SERVO_MAX_ANGLE) / pot_adc_limit;

        // Обчислення значення duty
        uint32_t duty = min_duty +
                        ((current_angle * (max_duty - min_duty)) / SERVO_MAX_ANGLE);

        if (current_angle != prev_angle)
        {
            ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, duty);
            ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);

            ESP_LOGI(TAG, "Кут відхилення від крайнього лівого положення: %d deg (ADC: %d, Duty: %lu)",
                     current_angle, raw_adc, duty);

            prev_angle = current_angle;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

#endif