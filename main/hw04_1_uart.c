#include "hw04_1_uart.h"

#if CONFIG_HW_04_1_UART_BRIDGE

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART_PORT UART_NUM_2
#define BUF_SIZE 1024

void hw04_1_run(void)
{
    // === 1. Ініціалізація UART2 на основі конфігурації Kconfig ===
    uart_config_t uart_config = {
        .baud_rate = CONFIG_HW_04_1_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT,
                 CONFIG_HW_04_1_UART_TX_GPIO,
                 CONFIG_HW_04_1_UART_RX_GPIO,
                 UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);

    // === 2. Ініціалізація GPIO (Світлодіод та Кнопка) ===
    gpio_reset_pin(CONFIG_HW_04_1_LED_GPIO);
    gpio_set_direction(CONFIG_HW_04_1_LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_reset_pin(CONFIG_HW_04_1_BTN_GPIO);
    gpio_set_direction(CONFIG_HW_04_1_BTN_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(CONFIG_HW_04_1_BTN_GPIO, GPIO_PULLUP_ONLY);

    // Змінні стану
    uint8_t rx_data = 0;
    uint32_t last_btn_change = 0;
    int last_raw_state = 1;
    int stable_btn_state = 1;
    uint32_t led_state = 0;

    // === 3. Головний неблокуючий цикл модуля ===
    while (1)
    {
        // --- 3.1. Обробка кнопки (Дебаунс через FreeRTOS ticks) ---
        int current_raw_state = gpio_get_level(CONFIG_HW_04_1_BTN_GPIO);
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // Фіксуємо зміну фізичного рівню на піні
        if (current_raw_state != last_raw_state)
        {
            last_btn_change = now;
            last_raw_state = current_raw_state;
        }

        // Перевіряємо стабільність сигналу протягом 50 мс
        if ((now - last_btn_change) > 50)
        {
            if (current_raw_state != stable_btn_state)
            {
                stable_btn_state = current_raw_state;

                // Натискання (перехід у LOW при Pull-Up)
                if (stable_btn_state == 0)
                {
                    char tx_msg = '1';
                    uart_write_bytes(UART_PORT, &tx_msg, 1);
                }
            }
        }

        // --- 3.2. Читання UART2 ---
        // Таймаут 10 мс не блокує цикл надовго
        int len = uart_read_bytes(UART_PORT, &rx_data, 1, 10 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            if (rx_data == '1' || rx_data == 't')
            {
                led_state = !led_state;
                gpio_set_level(CONFIG_HW_04_1_LED_GPIO, led_state);
            }
        }

        // Обов'язкова затримка для запобігання спрацьовування Task Watchdog Timer
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

#endif
