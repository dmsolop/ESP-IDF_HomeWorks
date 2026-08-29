#include "sdkconfig.h"

#if CONFIG_HW_03_3_PWM_CONTROL

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "PWM_CTRL";

// Налаштування таймера (5 кГц, 13 біт)
#define PWM_MODE LEDC_LOW_SPEED_MODE
#define PWM_TIMER LEDC_TIMER_0
#define PWM_FREQ_HZ 5000
#define PWM_RESOLUTION LEDC_TIMER_13_BIT
#define PWM_MAX_DUTY 8191 // (2^13 - 1)

#define LED_CHANNEL LEDC_CHANNEL_0
#define MOTOR_CHANNEL LEDC_CHANNEL_1

void hw03_3_run(void)
{
    // Налаштування спільного таймера
    ledc_timer_config_t timer_conf = {
        .speed_mode = PWM_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = PWM_TIMER,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    // Налаштування каналу світлодіода
    ledc_channel_config_t led_conf = {
        .speed_mode = PWM_MODE,
        .channel = LED_CHANNEL,
        .timer_sel = PWM_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = CONFIG_HW_03_3_LED_GPIO,
        .duty = 0,
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&led_conf));

    // Налаштування каналу мотора
    ledc_channel_config_t motor_conf = {
        .speed_mode = PWM_MODE,
        .channel = MOTOR_CHANNEL,
        .timer_sel = PWM_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = CONFIG_HW_03_3_MOTOR_GPIO,
        .duty = 0,
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&motor_conf));

    // Ініціалізація АЦП
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {.unit_id = ADC_UNIT_1};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t adc_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, CONFIG_HW_03_3_ADC_POT, &adc_cfg));

    ESP_LOGI(TAG, "PWM Control Started: LED (GPIO %d), Motor (GPIO %d)",
             CONFIG_HW_03_3_LED_GPIO, CONFIG_HW_03_3_MOTOR_GPIO);

    float ema_val = 0.0f;
    bool is_first_read = true;

    while (1)
    {
        int raw_adc = 0;
        if (adc_oneshot_read(adc_handle, CONFIG_HW_03_3_ADC_POT, &raw_adc) == ESP_OK)
        {

            // Простий фільтр EMA (альфа = 0.2)
            if (is_first_read)
            {
                ema_val = raw_adc;
                is_first_read = false;
            }
            else
            {
                ema_val = (0.2f * raw_adc) + (0.8f * ema_val);
            }

            // Масштабування: АЦП (12 біт: 0-4095) -> PWM (13 біт: 0-8191)
            uint32_t duty = (uint32_t)(ema_val * 2.0f);
            if (duty > PWM_MAX_DUTY)
                duty = PWM_MAX_DUTY;

            ledc_set_duty(PWM_MODE, LED_CHANNEL, duty);
            ledc_update_duty(PWM_MODE, LED_CHANNEL);

            ledc_set_duty(PWM_MODE, MOTOR_CHANNEL, duty);
            ledc_update_duty(PWM_MODE, MOTOR_CHANNEL);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

#endif