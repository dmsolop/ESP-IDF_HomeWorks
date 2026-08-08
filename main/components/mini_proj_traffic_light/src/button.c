#include "sdkconfig.h"

#if CONFIG_MINI_PROJ_TRAFFIC_LIGHT

#include "button.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "BUTTON_MODULE";

// Зберігаємо хендл таски світлофора для відправки нотифікації
static TaskHandle_t g_fsm_task_handle = NULL;

// Час останнього дійсно зафіксованого спрацьовування (в мікросекундах)
static volatile uint64_t g_last_isr_time = 0;

/**
 * @brief Обробник апаратного переривання (ISR) з програмним дебоунсом.
 */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    uint64_t current_time = esp_timer_get_time();

    // Програмний дебоунс: порівнюємо час у мікросекундах з Kconfig параметором
    if ((current_time - g_last_isr_time) < CONFIG_TRAFFIC_DEBOUNCE_TIME)
    {
        return; // Дребезг — миттєво виходимо з ISR
    }

    g_last_isr_time = current_time;

    if (g_fsm_task_handle != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        // Будимо головну таску FSM (відправляємо "пінок")
        vTaskNotifyGiveFromISR(g_fsm_task_handle, &xHigherPriorityTaskWoken);

        // Перемикаємо контекст, якщо таска FSM має вищий пріоритет
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

esp_err_t button_init(TaskHandle_t target_task)
{
    if (target_task == NULL)
    {
        ESP_LOGE(TAG, "Target task handle is NULL!");
        return ESP_ERR_INVALID_ARG;
    }

    g_fsm_task_handle = target_task;

    // Конфігурація GPIO під кнопку (з підтяжкою do +3.3V)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_TRAFFIC_PIN_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE // Спрацьовування при натисканні (перехід 1 -> 0)
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure GPIO %d", CONFIG_TRAFFIC_PIN_BUTTON);
        return err;
    }

    // Встановлюємо сервіс переривань (якщо ще не встановлений)
    gpio_install_isr_service(0);

    // Додаємо наш обробник для піна кнопки
    err = gpio_isr_handler_add(CONFIG_TRAFFIC_PIN_BUTTON, button_isr_handler, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add ISR handler for GPIO %d", CONFIG_TRAFFIC_PIN_BUTTON);
        return err;
    }

    ESP_LOGI(TAG, "Button initialized on GPIO %d with %d us debounce",
             CONFIG_TRAFFIC_PIN_BUTTON, CONFIG_TRAFFIC_DEBOUNCE_TIME);

    return ESP_OK;
}

#endif