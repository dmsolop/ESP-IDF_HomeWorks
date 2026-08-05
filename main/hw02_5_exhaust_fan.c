#include "hw02_5_exhaust_fan.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define FAN_GPIO GPIO_NUM_4
#define TAG "EXHAUST_FAN"

#ifdef CONFIG_EXHAUST_FAN_DEBUG_MODE
#define TIME_MULTIPLIER 1
#else
#define TIME_MULTIPLIER 60
#endif

typedef enum
{
    FAN_STATE_WORKING,
    FAN_STATE_PAUSED
} fan_state_t;

static gptimer_handle_t gptimer = NULL;
static TaskHandle_t fan_task_handle = NULL;
static fan_state_t current_state = FAN_STATE_WORKING;

// Апаратне переривання таймера (працює миттєво, будить таску через нотифікацію)
static bool IRAM_ATTR timer_alarm_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_awoken = pdFALSE;
    vTaskNotifyGiveFromISR(fan_task_handle, &high_task_awoken);
    return high_task_awoken == pdTRUE;
}

// Головна фонова таска керування вентилятором
void exhaust_fan_task(void *pvParameters)
{
    // 1. Налаштування GPIO для реле
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FAN_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    // 2. Налаштування апаратного таймера (GPTimer: 1 мікросекунда на 1 тік)
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_alarm_callback,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    // Розрахунок часу у мікросекундах
    uint64_t work_ticks = (uint64_t)CONFIG_EXHAUST_FAN_WORK_TIME_MIN * TIME_MULTIPLIER * 1000000ULL;
    uint64_t pause_ticks = (uint64_t)CONFIG_EXHAUST_FAN_PAUSE_TIME_MIN * TIME_MULTIPLIER * 1000000ULL;

    // 3. Запуск початкового стану (Робота)
    current_state = FAN_STATE_WORKING;
    gpio_set_level(FAN_GPIO, 1); // Увімкнути реле/вентилятор

    gptimer_alarm_config_t alarm_config = {
        .alarm_value = work_ticks,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = false,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    ESP_LOGI(TAG, "System started. State: WORKING. Duration: %d units", CONFIG_EXHAUST_FAN_WORK_TIME_MIN);

    while (1)
    {
        // Чекаємо на сповіщення від переривання (не споживає процесорний час)
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Перемикання станів та оновлення таймера «на льоту»
        if (current_state == FAN_STATE_WORKING)
        {
            current_state = FAN_STATE_PAUSED;
            gpio_set_level(FAN_GPIO, 0); // Вимкнути вентилятор

            gptimer_stop(gptimer);
            alarm_config.alarm_value = pause_ticks;
            gptimer_set_alarm_action(gptimer, &alarm_config);
            gptimer_set_raw_count(gptimer, 0);
            gptimer_start(gptimer);

            ESP_LOGI(TAG, "Timer expired. State changed: PAUSED. Duration: %d units", CONFIG_EXHAUST_FAN_PAUSE_TIME_MIN);
        }
        else
        {
            current_state = FAN_STATE_WORKING;
            gpio_set_level(FAN_GPIO, 1); // Увімкнути вентилятор

            gptimer_stop(gptimer);
            alarm_config.alarm_value = work_ticks;
            gptimer_set_alarm_action(gptimer, &alarm_config);
            gptimer_set_raw_count(gptimer, 0);
            gptimer_start(gptimer);

            ESP_LOGI(TAG, "Timer expired. State changed: WORKING. Duration: %d units", CONFIG_EXHAUST_FAN_WORK_TIME_MIN);
        }
    }
}

// Функція ініціалізації завдання для виклику з main.c
void exhaust_fan_init(void)
{
    // Зберігаємо поточний хендлер таски для ISR
    fan_task_handle = xTaskGetCurrentTaskHandle();

    // Створюємо окрему FreeRTOS таску
    xTaskCreate(exhaust_fan_task, "exhaust_fan_task", 2048, NULL, 5, NULL);
}