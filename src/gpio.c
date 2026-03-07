/**
 * @file gpio.c
 * @brief GPIO 模块实现
 */
#include "zyrthi/hal/gpio.h"
#include "zyrthi/adapter/esp32/config.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "zyrthi.gpio";

gpio_status_t gpio_mode(u8_t pin, pin_mode_t mode) {
    // 检查引脚范围
    if (pin >= ZYRTHI_GPIO_MAX_PIN) {
        ESP_LOGE(TAG, "Invalid pin: %d", pin);
        return GPIO_ERROR_INVALID_PIN;
    }

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << pin,
        .intr_type = GPIO_INTR_DISABLE,
    };

    switch (mode) {
        case OUTPUT:
            cfg.mode = GPIO_MODE_OUTPUT;
            cfg.pull_up_en = GPIO_PULLUP_DISABLE;
            cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case INPUT:
            cfg.mode = GPIO_MODE_INPUT;
            cfg.pull_up_en = GPIO_PULLUP_DISABLE;
            cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case INPUT_PULLUP:
            cfg.mode = GPIO_MODE_INPUT;
            cfg.pull_up_en = GPIO_PULLUP_ENABLE;
            cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case INPUT_PULLDOWN:
            cfg.mode = GPIO_MODE_INPUT;
            cfg.pull_up_en = GPIO_PULLUP_DISABLE;
            cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;
        default:
            ESP_LOGE(TAG, "Unsupported pin mode: %d", mode);
            return GPIO_ERROR_UNSUPPORTED;
    }

    esp_err_t ret = gpio_config(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
        return GPIO_ERROR_INVALID_PIN;
    }

    return GPIO_OK;
}

gpio_status_t gpio_write(u8_t pin, volt_t level) {
    if (pin >= ZYRTHI_GPIO_MAX_PIN) {
        return GPIO_ERROR_INVALID_PIN;
    }

    esp_err_t ret = gpio_set_level((gpio_num_t)pin, level == HIGH ? 1 : 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_set_level failed: %s", esp_err_to_name(ret));
        return GPIO_ERROR_INVALID_PIN;
    }

    return GPIO_OK;
}

gpio_result_t gpio_read(u8_t pin) {
    if (pin >= ZYRTHI_GPIO_MAX_PIN) {
        return (gpio_result_t){
            .status = GPIO_ERROR_INVALID_PIN,
            .volt = LOW
        };
    }

    int level = gpio_get_level((gpio_num_t)pin);
    return (gpio_result_t){
        .status = GPIO_OK,
        .volt = level ? HIGH : LOW
    };
}