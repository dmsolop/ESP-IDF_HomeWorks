#include "sdkconfig.h"

#if CONFIG_HW_03_1_ADC_CALIB

#include "hw03_1_adc_calib.h"
#include <stdio.h>
#include <math.h> // Потрібен для функції fabs()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "HW03_1";

void hw03_1_run(void)
{
    // 1. Ініціалізація ADC Oneshot
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12, // 12 дБ дозволяє міряти до ~3.3 В
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, CONFIG_HW_03_1_ADC_CHANNEL, &config));

    // 2. Ініціалізація Калібрування (Curve Fitting для ESP32-S3)
    adc_cali_handle_t cali_handle = NULL;
    bool is_calibrated = false;

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = CONFIG_HW_03_1_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    if (adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle) == ESP_OK)
    {
        is_calibrated = true;
        ESP_LOGI(TAG, "ADC Calibration successfully initialized");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to init ADC calibration!");
    }

    // 3. Друкуємо шапку таблиці (використовуємо printf для акуратного форматування)
    printf("\n");
    printf("RAW     U_manual(mV)   U_cali(mV)   Error(%%)\n");
    printf("----------------------------------------------\n");

    // 4. Основний цикл вимірювань
    while (1)
    {
        int raw_val = 0;
        int u_cali_mv = 0;

        if (adc_oneshot_read(adc_handle, CONFIG_HW_03_1_ADC_CHANNEL, &raw_val) == ESP_OK)
        {

            // Розрахунок "Вручну"
            float u_manual_mv = (raw_val * (float)CONFIG_HW_03_1_VREF_NOMINAL_MV) / 4095.0f;

            // Отримання каліброваних мілівольт
            if (is_calibrated)
            {
                adc_cali_raw_to_voltage(cali_handle, raw_val, &u_cali_mv);
            }
            else
            {
                u_cali_mv = (int)u_manual_mv; // Запасний варіант, якщо калібрування збійнуло
            }

            // Розрахунок похибки у відсотках із захистом від ділення на нуль
            float error_pct = 0.0f;
            if (u_cali_mv > 0)
            {
                error_pct = (fabs(u_manual_mv - (float)u_cali_mv) / (float)u_cali_mv) * 100.0f;
            }
            else if (u_cali_mv == 0 && u_manual_mv > 0)
            {
                // На випадок невеликого офсету біля 0 мВ
                error_pct = 100.0f;
            }

            // Вивід рядка даних з вирівнюванням
            printf("%-7d %-14.1f %-12d %-8.2f\n", raw_val, u_manual_mv, u_cali_mv, error_pct);
        }

        // Затримка ~100 мс, як вказано в ДЗ (значення беремо з Kconfig)
        vTaskDelay(pdMS_TO_TICKS(CONFIG_HW_03_1_READ_INTERVAL_MS));
    }
}

#endif // CONFIG_HW_03_1_ADC_CALIB