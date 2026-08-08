#include "sdkconfig.h"

#if CONFIG_MINI_PROJ_TRAFFIC_LIGHT

#include "traffic_fsm.h"
#include "esp_log.h"

static const char *TAG = "TRAFFIC_FSM";

esp_err_t traffic_fsm_init(void)
{
    ESP_LOGI(TAG, "Traffic FSM initialized");
    return ESP_OK;
}

#endif