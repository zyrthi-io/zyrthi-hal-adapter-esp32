/**
 * @file pwm.c
 * @brief PWM 模块实现（基于 LEDC）
 */

#include "zyrthi/adapter/esp32/config.h"
#include "zyrthi/hal/def.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "zyrthi.pwm";

#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_TIMER_BIT LEDC_TIMER_13_BIT
#define LEDC_MAX_DUTY ((1 << 13) - 1)

typedef struct { bool in_use; gpio_num_t pin; ledc_channel_t ch; ledc_timer_t tim; u32_t freq, duty; } pwm_ch_t;
static pwm_ch_t pwm_ch[ZYRTHI_PWM_MAX_CHANNELS];
static bool timers_init[ZYRTHI_PWM_MAX_TIMERS] = {0};

#include "zyrthi/hal/pwm.h"

pwm_status_t pwm_open(u8_t pin, pwm_config_t cfg) {
    int ch = -1;
    for (int i = 0; i < ZYRTHI_PWM_MAX_CHANNELS; i++) if (!pwm_ch[i].in_use) { ch = i; break; }
    if (ch < 0) { ESP_LOGE(TAG, "No free channels"); return PWM_ERROR_UNSUPPORTED; }

    ledc_timer_t tim = (ledc_timer_t)(ch / 4);
    if (!timers_init[tim]) {
        ledc_timer_config_t t = {
            .speed_mode = LEDC_MODE,
            .duty_resolution = LEDC_TIMER_BIT,
            .timer_num = tim,
            .freq_hz = cfg.freq_hz,
            .clk_cfg = LEDC_AUTO_CLK,
        };
        if (ledc_timer_config(&t) != ESP_OK) return PWM_ERROR_INVALID_ARG;
        timers_init[tim] = true;
    }

    u32_t d = (cfg.duty > 10000) ? 10000 : cfg.duty;
    ledc_channel_config_t c = {
        .gpio_num = (gpio_num_t)pin,
        .speed_mode = LEDC_MODE,
        .channel = (ledc_channel_t)ch,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = tim,
        .duty = (d * LEDC_MAX_DUTY) / 10000,
        .hpoint = 0,
    };
    if (ledc_channel_config(&c) != ESP_OK) return PWM_ERROR_INVALID_PIN;

    pwm_ch[ch] = (pwm_ch_t){true, (gpio_num_t)pin, (ledc_channel_t)ch, tim, cfg.freq_hz, d};
    ESP_LOGI(TAG, "PWM opened: pin=%d, freq=%lu", pin, (unsigned long)cfg.freq_hz);
    return PWM_OK;
}

pwm_status_t pwm_close(u8_t pin) {
    for (int i = 0; i < ZYRTHI_PWM_MAX_CHANNELS; i++)
        if (pwm_ch[i].in_use && pwm_ch[i].pin == pin) {
            ledc_stop(LEDC_MODE, pwm_ch[i].ch, 0);
            pwm_ch[i].in_use = false;
            ESP_LOGI(TAG, "PWM closed: pin=%d", pin);
            return PWM_OK;
        }
    return PWM_ERROR_INVALID_PIN;
}

pwm_status_t pwm_write(u8_t pin, u32_t duty) {
    for (int i = 0; i < ZYRTHI_PWM_MAX_CHANNELS; i++)
        if (pwm_ch[i].in_use && pwm_ch[i].pin == pin) {
            if (duty > 10000) duty = 10000;
            ledc_set_duty(LEDC_MODE, pwm_ch[i].ch, (duty * LEDC_MAX_DUTY) / 10000);
            ledc_update_duty(LEDC_MODE, pwm_ch[i].ch);
            pwm_ch[i].duty = duty;
            return PWM_OK;
        }
    return PWM_ERROR_INVALID_PIN;
}

pwm_result_t pwm_read(u8_t pin) {
    for (int i = 0; i < ZYRTHI_PWM_MAX_CHANNELS; i++)
        if (pwm_ch[i].in_use && pwm_ch[i].pin == pin)
            return (pwm_result_t){PWM_OK, pwm_ch[i].duty};
    return (pwm_result_t){PWM_ERROR_INVALID_PIN, 0};
}
