#ifndef BUZZER_H
#define BUZZER_H

#include "sdkconfig.h"

#if CONFIG_MINI_PROJ_TRAFFIC_LIGHT

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef enum
{
    BUZZER_CMD_STOP = 0,
    BUZZER_CMD_PEDESTRIAN_WALK,
    BUZZER_CMD_WARNING
} buzzer_cmd_t;

esp_err_t buzzer_init(TaskHandle_t *out_buzzer_task_handle);
void buzzer_send_cmd(buzzer_cmd_t cmd);

#endif
#endif