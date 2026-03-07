/**
 * @file system.c
 * @brief 系统初始化实现
 */
#include "zyrthi/hal/system.h"
#include "zyrthi/adapter/esp32/config.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "zyrthi.system";

sys_status_t system_init(void) {
    // 初始化 ESP Timer（用于 millis/micros）
    esp_err_t ret = esp_timer_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to init esp_timer: %s", esp_err_to_name(ret));
        return SYS_ERROR_UNSUPPORTED;
    }

    ESP_LOGI(TAG, "System initialized (chip: %s)", ZYRTHI_CHIP_NAME);
    return SYS_OK;
}
