#include "sdkconfig.h"

// Додано підтримку конфіга нової домашки
#if defined(CONFIG_MINI_PROJ_TRAFFIC_LIGHT) || defined(CONFIG_HW_03_2_ADC_EMA)

#include "sensor_ldr.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

static const char *TAG = "SENSOR_LDR";
static adc_oneshot_unit_handle_t g_adc_handle = NULL;
static adc_cali_handle_t g_cali_handle = NULL;
static bool g_is_calibrated = false;

// Змінні для фільтра та кешування даних
static float g_ema_filtered_mv = -1.0f;
static int g_last_raw_val = 0;
static ldr_mode_t g_current_mode = LDR_MODE_DAY;

#define ALPHA_SLOW 0.05f
#define ALPHA_FAST 0.80f
#define DELTA_THRESHOLD_MV 150

// Фонова таска замінює синхронне зчитування для правильної роботи EMA
static void ldr_processing_task(void *arg)
{
    while (1)
    {
        int raw_val = 0;
        if (adc_oneshot_read(g_adc_handle, CONFIG_TRAFFIC_ADC_CHANNEL_LDR, &raw_val) == ESP_OK)
        {
            g_last_raw_val = raw_val; // Зберігаємо для зворотної сумісності з sensor_ldr_read_raw()

            int voltage_mv = 0;
            if (g_is_calibrated)
            {
                adc_cali_raw_to_voltage(g_cali_handle, raw_val, &voltage_mv);
            }
            else
            {
                voltage_mv = (raw_val * 3300) / 4095;
            }

            if (g_ema_filtered_mv < 0.0f)
            {
                g_ema_filtered_mv = (float)voltage_mv;
            }

            float delta = abs(voltage_mv - (int)g_ema_filtered_mv);
            float alpha = (delta > DELTA_THRESHOLD_MV) ? ALPHA_FAST : ALPHA_SLOW;
            g_ema_filtered_mv = (alpha * (float)voltage_mv) + ((1.0f - alpha) * g_ema_filtered_mv);

            int filtered_mv = (int)g_ema_filtered_mv;

            if (g_current_mode == LDR_MODE_DAY && filtered_mv < CONFIG_TRAFFIC_LDR_NIGHT_THRESHOLD)
            {
                g_current_mode = LDR_MODE_NIGHT;
            }
            else if (g_current_mode == LDR_MODE_NIGHT && filtered_mv > CONFIG_TRAFFIC_LDR_DAY_THRESHOLD)
            {
                g_current_mode = LDR_MODE_DAY;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

esp_err_t sensor_ldr_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_config, &g_adc_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to init ADC unit");
        return err;
    }

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    err = adc_oneshot_config_channel(g_adc_handle, CONFIG_TRAFFIC_ADC_CHANNEL_LDR, &config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to config ADC channel %d", CONFIG_TRAFFIC_ADC_CHANNEL_LDR);
        return err;
    }

    // Ініціалізація калібрування
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = CONFIG_TRAFFIC_ADC_CHANNEL_LDR,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &g_cali_handle) == ESP_OK)
    {
        g_is_calibrated = true;
    }

    // Запуск фільтрації
    xTaskCreate(ldr_processing_task, "ldr_task", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "LDR initialized on ADC channel %d", CONFIG_TRAFFIC_ADC_CHANNEL_LDR);
    return ESP_OK;
}

// Залишаємо оригінальну функцію, але тепер вона повертає закешоване значення з таски
int sensor_ldr_read_raw(void)
{
    if (g_adc_handle == NULL)
    {
        return -1;
    }
    return g_last_raw_val;
}

// Додаємо нову функцію для отримання відфільтрованої напруги
int sensor_ldr_get_filtered_mv(void)
{
    return (int)g_ema_filtered_mv;
}

// Тепер функція просто віддає готовий стан, який розрахувала таска з гістерезисом
ldr_mode_t sensor_ldr_get_mode(void)
{
    return g_current_mode;
}

#endif

// #include "sdkconfig.h"

// #if CONFIG_MINI_PROJ_TRAFFIC_LIGHT

// #include "sensor_ldr.h"
// #include "esp_adc/adc_oneshot.h"
// #include "esp_log.h"

// static const char *TAG = "SENSOR_LDR";
// static adc_oneshot_unit_handle_t g_adc_handle = NULL;

// esp_err_t sensor_ldr_init(void)
// {
//     adc_oneshot_unit_init_cfg_t init_config = {
//         .unit_id = ADC_UNIT_1,
//     };
//     esp_err_t err = adc_oneshot_new_unit(&init_config, &g_adc_handle);
//     if (err != ESP_OK)
//     {
//         ESP_LOGE(TAG, "Failed to init ADC unit");
//         return err;
//     }

//     adc_oneshot_chan_cfg_t config = {
//         .bitwidth = ADC_BITWIDTH_DEFAULT,
//         .atten = ADC_ATTEN_DB_12,
//     };
//     err = adc_oneshot_config_channel(g_adc_handle, CONFIG_TRAFFIC_ADC_CHANNEL_LDR, &config);
//     if (err != ESP_OK)
//     {
//         ESP_LOGE(TAG, "Failed to config ADC channel %d", CONFIG_TRAFFIC_ADC_CHANNEL_LDR);
//         return err;
//     }

//     ESP_LOGI(TAG, "LDR initialized on ADC channel %d", CONFIG_TRAFFIC_ADC_CHANNEL_LDR);
//     return ESP_OK;
// }

// int sensor_ldr_read_raw(void)
// {
//     if (g_adc_handle == NULL)
//     {
//         return -1;
//     }
//     int raw_val = 0;
//     if (adc_oneshot_read(g_adc_handle, CONFIG_TRAFFIC_ADC_CHANNEL_LDR, &raw_val) == ESP_OK)
//     {
//         return raw_val;
//     }
//     return -1;
// }

// ldr_mode_t sensor_ldr_get_mode(void)
// {
//     static ldr_mode_t current_mode = LDR_MODE_DAY;
//     int raw = sensor_ldr_read_raw();

//     if (raw < 0)
//         return current_mode;

//     if (raw < CONFIG_TRAFFIC_LDR_NIGHT_THRESHOLD)
//     {
//         current_mode = LDR_MODE_NIGHT;
//     }
//     else if (raw > CONFIG_TRAFFIC_LDR_DAY_THRESHOLD)
//     {
//         current_mode = LDR_MODE_DAY;
//     }
//     // Якщо значення між порогами — стан не змінюється (гістерезис)

//     return current_mode;
// }

// #endif