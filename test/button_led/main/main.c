/**
 * @file main.c
 * @brief 按键控制 LED 示例 (ESP32-S3-N16R8)
 */
#include "zyrthi_hal.h"
#include "zyrthi/hal/gpio.h"
#include "zyrthi/hal/time.h"
#include "esp_log.h"

static const char *TAG = "main";

// 引脚定义 (根据你的板子修改)
#define LED_PIN     2       // 板载 LED (通常 GPIO2)
#define BUTTON_PIN  0       // BOOT 按键 (GPIO0)

void app_main(void) {
    ESP_LOGI(TAG, "=== Button LED Demo (ESP32-S3) ===");

    // 初始化系统
    system_init();

    // 配置 LED 为输出
    gpio_mode(LED_PIN, OUTPUT);
    gpio_write(LED_PIN, LOW);

    // 配置按键为输入 (内部上拉)
    gpio_mode(BUTTON_PIN, INPUT_PULLUP);

    ESP_LOGI(TAG, "Press BOOT button to toggle LED");

    bool led_state = false;
    bool last_button = HIGH;

    while (1) {
        // 读取按键状态
        gpio_result_t btn = gpio_read(BUTTON_PIN);

        // 检测下降沿 (按键按下)
        if (last_button == HIGH && btn.volt == LOW) {
            // 消抖延时
            delay_ms(50);
            
            // 再次确认按键状态
            btn = gpio_read(BUTTON_PIN);
            if (btn.volt == LOW) {
                // 切换 LED 状态
                led_state = !led_state;
                gpio_write(LED_PIN, led_state ? HIGH : LOW);
                ESP_LOGI(TAG, "LED: %s", led_state ? "ON" : "OFF");
            }
        }

        last_button = btn.volt;
        delay_ms(10);  // 轮询间隔
    }
}