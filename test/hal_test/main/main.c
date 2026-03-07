/**
 * @file main.c
 * @brief HAL 测试程序
 */
#include <stdio.h>
#include "zyrthi_hal.h"
#include "zyrthi/adapter/esp32/config.h"
#include "zyrthi/hal/gpio.h"
#include "zyrthi/hal/time.h"
#include "zyrthi/hal/uart.h"
#include "zyrthi/hal/adc.h"
#include "zyrthi/hal/pwm.h"
#include "zyrthi/hal/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "hal_test";

// 测试引脚配置（根据硬件修改）
#define LED_PIN         2       // 板载 LED（ESP32）
#define BUTTON_PIN      0       // BOOT 按钮
#define ADC_PIN         34      // ADC1_CH6
#define PWM_PIN         18      // PWM 输出

// 测试函数声明
static void test_system(void);
static void test_time(void);
static void test_gpio(void);
static void test_uart(void);
static void test_adc(void);
static void test_pwm(void);
static void test_i2c(void);

void app_main(void) {
    ESP_LOGI(TAG, "=== ZYRTHI HAL Test Start ===");
    ESP_LOGI(TAG, "Chip: %s", ZYRTHI_CHIP_NAME);

    // 1. 系统初始化
    test_system();

    // 2. 时间模块测试
    test_time();

    // 3. GPIO 测试
    test_gpio();

    // 4. UART 测试
    test_uart();

    // 5. ADC 测试
    test_adc();

    // 6. PWM 测试
    test_pwm();

    // 7. I2C 测试
    test_i2c();

    ESP_LOGI(TAG, "=== All tests completed ===");
}

static void test_system(void) {
    ESP_LOGI(TAG, "--- System Test ---");
    
    sys_status_t status = system_init();
    if (status == SYS_OK) {
        ESP_LOGI(TAG, "system_init: PASS");
    } else {
        ESP_LOGE(TAG, "system_init: FAIL (status=%d)", status);
    }
}

static void test_time(void) {
    ESP_LOGI(TAG, "--- Time Test ---");
    
    // millis 测试
    time_result_t t1 = millis();
    vTaskDelay(pdMS_TO_TICKS(100));
    time_result_t t2 = millis();
    
    if (t2.value > t1.value && (t2.value - t1.value) >= 100) {
        ESP_LOGI(TAG, "millis: PASS (delta=%u ms)", t2.value - t1.value);
    } else {
        ESP_LOGE(TAG, "millis: FAIL");
    }

    // micros 测试
    time_result_t us1 = micros();
    time_result_t us2 = micros();
    ESP_LOGI(TAG, "micros: %u -> %u", us1.value, us2.value);

    // delay 测试
    t1 = millis();
    delay_ms(50);
    t2 = millis();
    ESP_LOGI(TAG, "delay_ms(50): actual=%u ms", t2.value - t1.value);
}

static void test_gpio(void) {
    ESP_LOGI(TAG, "--- GPIO Test ---");
    
    // LED 输出测试
    gpio_status_t status = gpio_mode(LED_PIN, OUTPUT);
    if (status != GPIO_OK) {
        ESP_LOGE(TAG, "gpio_mode(LED, OUTPUT): FAIL");
        return;
    }
    ESP_LOGI(TAG, "gpio_mode(LED, OUTPUT): PASS");

    // LED 闪烁 3 次
    for (int i = 0; i < 6; i++) {
        gpio_write(LED_PIN, i % 2 ? HIGH : LOW);
        delay_ms(200);
    }
    gpio_write(LED_PIN, LOW);
    ESP_LOGI(TAG, "LED blink: PASS");

    // 按钮输入测试
    status = gpio_mode(BUTTON_PIN, INPUT_PULLUP);
    if (status != GPIO_OK) {
        ESP_LOGW(TAG, "gpio_mode(BUTTON, INPUT_PULLUP): FAIL (may not exist)");
    } else {
        gpio_result_t btn = gpio_read(BUTTON_PIN);
        ESP_LOGI(TAG, "Button state: %s", btn.volt == HIGH ? "HIGH" : "LOW");
    }
}

static void test_uart(void) {
    ESP_LOGI(TAG, "--- UART Test ---");
    
    uart_hdl_t uart = uart_default();
    if (uart == NULL) {
        ESP_LOGE(TAG, "uart_default: FAIL");
        return;
    }
    ESP_LOGI(TAG, "uart_default: PASS");

    uart_status_t status = uart_init(uart, 115200);
    if (status != UART_OK) {
        ESP_LOGE(TAG, "uart_init: FAIL (status=%d)", status);
        return;
    }
    ESP_LOGI(TAG, "uart_init: PASS");

    // 发送测试
    uart_result_t result = uart_puts(uart, "Hello from ZYRTHI HAL!\r\n");
    if (result.status == UART_OK) {
        ESP_LOGI(TAG, "uart_puts: PASS (len=%u)", result.data.len);
    } else {
        ESP_LOGE(TAG, "uart_puts: FAIL");
    }
}

static void test_adc(void) {
    ESP_LOGI(TAG, "--- ADC Test ---");
    
    adc_result_t result = adc_read(ADC_PIN);
    if (result.status == ADC_OK) {
        ESP_LOGI(TAG, "adc_read(pin=%d): raw=%u, voltage=%u mV", 
                 ADC_PIN, result.raw, result.volt_mv);
    } else {
        ESP_LOGE(TAG, "adc_read: FAIL (status=%d)", result.status);
    }
}

static void test_pwm(void) {
    ESP_LOGI(TAG, "--- PWM Test ---");
    
    pwm_status_t status = pwm_init(PWM_PIN, 5000);
    if (status != PWM_OK) {
        ESP_LOGE(TAG, "pwm_init: FAIL (status=%d)", status);
        return;
    }
    ESP_LOGI(TAG, "pwm_init: PASS");

    // 呼吸灯效果
    ESP_LOGI(TAG, "PWM breathing...");
    for (int i = 0; i <= 100; i += 5) {
        pwm_write(PWM_PIN, i * 100);  // 0-10000
        delay_ms(50);
    }
    for (int i = 100; i >= 0; i -= 5) {
        pwm_write(PWM_PIN, i * 100);
        delay_ms(50);
    }

    pwm_write(PWM_PIN, 0);
    ESP_LOGI(TAG, "PWM test: PASS");
}

static void test_i2c(void) {
    ESP_LOGI(TAG, "--- I2C Test ---");
    
    i2c_hdl_t i2c = i2c_default();
    if (i2c == NULL) {
        ESP_LOGE(TAG, "i2c_default: FAIL");
        return;
    }
    ESP_LOGI(TAG, "i2c_default: PASS");

    i2c_status_t status = i2c_init(i2c, 100000);
    if (status != I2C_OK) {
        ESP_LOGE(TAG, "i2c_init: FAIL (status=%d)", status);
        return;
    }
    ESP_LOGI(TAG, "i2c_init: PASS");

    // I2C 设备扫描
    ESP_LOGI(TAG, "I2C bus scan:");
    for (u8_t addr = 0x08; addr < 0x78; addr++) {
        status = i2c_begin(i2c, addr);
        if (status == I2C_OK) {
            // 尝试读取 0 字节来检测设备
            i2c_result_t result = i2c_read(i2c, NULL, 0);
            if (result.status == I2C_OK) {
                ESP_LOGI(TAG, "  Device found at 0x%02X", addr);
            }
            i2c_end(i2c);
        }
    }
    ESP_LOGI(TAG, "I2C scan complete");
}
