/**
 * @file pwm.c
 * @brief PWM 模块实现（基于 LEDC，多实例支持）
 */
#include "zyrthi/hal/pwm.h"
#include "zyrthi/adapter/esp32/config.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "zyrthi.pwm";

// PWM 通道上下文
typedef struct {
    bool in_use;
    gpio_num_t pin;
    ledc_channel_t channel;
    ledc_timer_t timer;
    u32_t freq_hz;
    u32_t duty;  // 0-10000
} pwm_ctx_t;

// PWM 实例池
static pwm_ctx_t pwm_channels[ZYRTHI_PWM_MAX_CHANNELS];
static bool timers_initialized[ZYRTHI_PWM_MAX_TIMERS] = {false};

// LEDC 配置
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_TIMER_BIT      LEDC_TIMER_13_BIT  // 13-bit resolution (0-8191)
#define LEDC_MAX_DUTY       ((1 << 13) - 1)    // 8191

pwm_status_t pwm_init(u8_t pin, u32_t freq_hz) {
    // 查找空闲通道
    int free_channel = -1;
    for (int i = 0; i < ZYRTHI_PWM_MAX_CHANNELS; i++) {
        if (!pwm_channels[i].in_use) {
            free_channel = i;
            break;
        }
    }

    if (free_channel < 0) {
        ESP_LOGE(TAG, "No free PWM channels");
        return PWM_ERROR_UNSUPPORTED;
    }

    // 分配定时器（每4个通道共享一个定时器）
    ledc_timer_t timer = (ledc_timer_t)(free_channel / 4);

    // 初始化定时器（如果尚未初始化）
    if (!timers_initialized[timer]) {
        ledc_timer_config_t timer_cfg = {
            .speed_mode = LEDC_MODE,
            .duty_resolution = LEDC_TIMER_BIT,
            .timer_num = timer,
            .freq_hz = freq_hz,
            .clk_cfg = LEDC_AUTO_CLK,
        };
        
        esp_err_t ret = ledc_timer_config(&timer_cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(ret));
            return PWM_ERROR_INVALID_ARG;
        }
        timers_initialized[timer] = true;
    }

    // 配置通道
    ledc_channel_config_t ch_cfg = {
        .gpio_num = (gpio_num_t)pin,
        .speed_mode = LEDC_MODE,
        .channel = (ledc_channel_t)free_channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = timer,
        .duty = 0,
        .hpoint = 0,
    };

    esp_err_t ret = ledc_channel_config(&ch_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(ret));
        return PWM_ERROR_INVALID_PIN;
    }

    // 保存上下文
    pwm_channels[free_channel].in_use = true;
    pwm_channels[free_channel].pin = (gpio_num_t)pin;
    pwm_channels[free_channel].channel = (ledc_channel_t)free_channel;
    pwm_channels[free_channel].timer = timer;
    pwm_channels[free_channel].freq_hz = freq_hz;
    pwm_channels[free_channel].duty = 0;

    ESP_LOGI(TAG, "PWM initialized: pin=%d, freq=%d Hz, channel=%d", pin, freq_hz, free_channel);
    return PWM_OK;
}

static pwm_ctx_t* find_channel_by_pin(u8_t pin) {
    for (int i = 0; i < ZYRTHI_PWM_MAX_CHANNELS; i++) {
        if (pwm_channels[i].in_use && pwm_channels[i].pin == pin) {
            return &pwm_channels[i];
        }
    }
    return NULL;
}

pwm_status_t pwm_write(u8_t pin, u32_t duty) {
    pwm_ctx_t *ctx = find_channel_by_pin(pin);
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Pin %d not initialized as PWM", pin);
        return PWM_ERROR_INVALID_PIN;
    }

    if (duty > 10000) {
        duty = 10000;  // clamp
    }

    // 转换 duty: 0-10000 -> 0-LEDC_MAX_DUTY
    u32_t ledc_duty = (duty * LEDC_MAX_DUTY) / 10000;

    esp_err_t ret = ledc_set_duty(LEDC_MODE, ctx->channel, ledc_duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_set_duty failed: %s", esp_err_to_name(ret));
        return PWM_ERROR_INVALID_ARG;
    }

    ret = ledc_update_duty(LEDC_MODE, ctx->channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_update_duty failed: %s", esp_err_to_name(ret));
        return PWM_ERROR_INVALID_ARG;
    }

    ctx->duty = duty;
    return PWM_OK;
}

pwm_result_t pwm_read(u8_t pin) {
    pwm_result_t result = { .status = PWM_ERROR_INVALID_PIN, .duty = 0 };

    pwm_ctx_t *ctx = find_channel_by_pin(pin);
    if (ctx == NULL) {
        return result;
    }

    result.status = PWM_OK;
    result.duty = ctx->duty;
    return result;
}
