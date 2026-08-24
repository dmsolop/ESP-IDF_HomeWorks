#ifndef SENSOR_LDR_H
#define SENSOR_LDR_H

#include "sdkconfig.h"

#if CONFIG_MINI_PROJ_TRAFFIC_LIGHT

#include "esp_err.h"
#include <stdbool.h>

typedef enum
{
    LDR_MODE_DAY = 0,
    LDR_MODE_NIGHT
} ldr_mode_t;

esp_err_t sensor_ldr_init(void);
int sensor_ldr_read_raw(void);
int sensor_ldr_get_filtered_mv(void); // Додано
ldr_mode_t sensor_ldr_get_mode(void);

#endif
#endif