#include "sdkconfig.h"

#if CONFIG_HW_03_4_BUZZER_PWM

#include <stdio.h>
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "hw03_4_buzzer.h"

static const char *TAG = "BUZZER_PLAYER";

#define BUZZER_MODE LEDC_LOW_SPEED_MODE
#define BUZZER_TIMER LEDC_TIMER_0
#define BUZZER_CHANNEL LEDC_CHANNEL_0
#define BUZZER_RESOLUTION LEDC_TIMER_10_BIT

#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_REST 0

typedef struct
{
    uint16_t freq_hz;
    uint8_t duration_ticks;
} note_t;
static const note_t melody[] = {
    // E E E | E E E | E G C D | E
    {NOTE_E4, 4},
    {NOTE_REST, 1},
    {NOTE_E4, 4},
    {NOTE_REST, 1},
    {NOTE_E4, 8},
    {NOTE_REST, 2},
    {NOTE_E4, 4},
    {NOTE_REST, 1},
    {NOTE_E4, 4},
    {NOTE_REST, 1},
    {NOTE_E4, 8},
    {NOTE_REST, 2},
    {NOTE_E4, 4},
    {NOTE_REST, 1},
    {NOTE_G4, 4},
    {NOTE_REST, 1},
    {NOTE_C4, 6},
    {NOTE_D4, 2},
    {NOTE_E4, 12},
    {NOTE_REST, 4},

    // F F F F | F E E E | E D D E | D G
    {NOTE_F4, 4},
    {NOTE_REST, 1},
    {NOTE_F4, 4},
    {NOTE_REST, 1},
    {NOTE_F4, 6},
    {NOTE_F4, 2},
    {NOTE_F4, 4},
    {NOTE_REST, 1},
    {NOTE_E4, 4},
    {NOTE_REST, 1},
    {NOTE_E4, 4},
    {NOTE_E4, 2},
    {NOTE_E4, 2},
    {NOTE_E4, 4},
    {NOTE_REST, 1},
    {NOTE_D4, 4},
    {NOTE_REST, 1},
    {NOTE_D4, 4},
    {NOTE_E4, 4},
    {NOTE_D4, 8},
    {NOTE_G4, 8},
    {NOTE_REST, 6}};

static const size_t MELODY_LEN = sizeof(melody) / sizeof(note_t);

// Стан плеєра
static size_t current_note_idx = 0;
static uint8_t ticks_passed = 0;
static bool is_playing = true;

// Зміна частоти на льоту
static void buzzer_set_tone(uint16_t freq_hz)
{
    if (freq_hz == NOTE_REST)
    {
        // Пауза
        ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, 0);
        ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL);
    }
    else
    {
        // Оновлюємо частоту таймера
        ledc_set_freq(BUZZER_MODE, BUZZER_TIMER, freq_hz);
        ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, 512);
        ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL);
    }
}

// Callback таймера (викликається кожні 50 мс)
static void timer_tick_callback(void *arg)
{
    if (!is_playing)
        return;

    // Якщо це початок нової ноти — вмикаємо її tone
    if (ticks_passed == 0)
    {
        buzzer_set_tone(melody[current_note_idx].freq_hz);
    }

    ticks_passed++;

    // Якщо нота відлунала свою тривалість
    if (ticks_passed >= melody[current_note_idx].duration_ticks)
    {
        ticks_passed = 0;
        current_note_idx++;

        // Зациклення мелодії
        if (current_note_idx >= MELODY_LEN)
        {
            current_note_idx = 0;
        }
    }
}

void hw03_4_run(void)
{
    // Конфігурація LEDC таймера
    ledc_timer_config_t timer_conf = {
        .speed_mode = BUZZER_MODE,
        .duty_resolution = BUZZER_RESOLUTION,
        .timer_num = BUZZER_TIMER,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    // Конфігурація LEDC каналу
    ledc_channel_config_t channel_conf = {
        .speed_mode = BUZZER_MODE,
        .channel = BUZZER_CHANNEL,
        .timer_sel = BUZZER_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = CONFIG_HW_03_4_BUZZER_GPIO,
        .duty = 0,
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    // Створення періодичного неблокуючого таймера на 50000 мкс
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_tick_callback,
        .name = "buzzer_tick_timer"};
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 50000)); // 50 000 мкс = 50 мс

    ESP_LOGI(TAG, "Неблокуючий плеєр запущено на GPIO %d", CONFIG_HW_03_4_BUZZER_GPIO);
}

#endif