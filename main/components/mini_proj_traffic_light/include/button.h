#ifndef BUTTON_H
#define BUTTON_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if CONFIG_MINI_PROJ_TRAFFIC_LIGHT

/**
 * @brief Ініціалізація GPIO кнопки пішохода та налаштування переривання.
 *
 * @param target_task Хендл таски (головної FSM), якій надсилатиметься нотифікація при натисканні.
 * @return esp_err_t ESP_OK при успішному налаштуванні.
 */
esp_err_t button_init(TaskHandle_t target_task);

#endif
#endif