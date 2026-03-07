/**
 * @file time.c
 * @brief 时间模块实现（millis/micros/delay）
 */
#include "zyrthi/hal/time.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

time_result_t millis(void) {
    int64_t us = esp_timer_get_time();
    return (time_result_t){
        .status = TIME_OK,
        .value = (u32_t)(us / 1000)
    };
}

time_result_t micros(void) {
    int64_t us = esp_timer_get_time();
    return (time_result_t){
        .status = TIME_OK,
        .value = (u32_t)(us & 0xFFFFFFFF)
    };
}

time_status_t delay_ms(u32_t ms) {
    if (ms == 0) {
        return TIME_OK;
    }
    // FreeRTOS tick 延时
    vTaskDelay(pdMS_TO_TICKS(ms));
    return TIME_OK;
}

time_status_t delay_us(u32_t us) {
    if (us == 0) {
        return TIME_OK;
    }
    
    // 短延时使用 esp_timer 的精确延时
    if (us < 1000) {
        int64_t start = esp_timer_get_time();
        while ((esp_timer_get_time() - start) < us) {
            // 忙等待
        }
    } else {
        // 长延时使用 vTaskDelay
        vTaskDelay(pdMS_TO_TICKS(us / 1000));
        // 处理剩余微秒
        u32_t remaining_us = us % 1000;
        if (remaining_us > 0) {
            int64_t start = esp_timer_get_time();
            while ((esp_timer_get_time() - start) < remaining_us) {
                // 忙等待
            }
        }
    }
    
    return TIME_OK;
}
