#include "sdkconfig.h"

#if CONFIG_MINI_PROJ_TRAFFIC_LIGHT

#include "sensor_ldr.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "SENSOR_LDR";
static adc_oneshot_unit_handle_t g_adc_handle = NULL;

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

    ESP_LOGI(TAG, "LDR initialized on ADC channel %d", CONFIG_TRAFFIC_ADC_CHANNEL_LDR);
    return ESP_OK;
}

int sensor_ldr_read_raw(void)
{
    if (g_adc_handle == NULL)
    {
        return -1;
    }
    int raw_val = 0;
    if (adc_oneshot_read(g_adc_handle, CONFIG_TRAFFIC_ADC_CHANNEL_LDR, &raw_val) == ESP_OK)
    {
        return raw_val;
    }
    return -1;
}

ldr_mode_t sensor_ldr_get_mode(void)
{
    static ldr_mode_t current_mode = LDR_MODE_DAY;
    int raw = sensor_ldr_read_raw();

    if (raw < 0)
        return current_mode;

    if (raw < CONFIG_TRAFFIC_LDR_NIGHT_THRESHOLD)
    {
        current_mode = LDR_MODE_NIGHT;
    }
    else if (raw > CONFIG_TRAFFIC_LDR_DAY_THRESHOLD)
    {
        current_mode = LDR_MODE_DAY;
    }
    // Якщо значення між порогами — стан не змінюється (гістерезис)

    return current_mode;
}

#endif