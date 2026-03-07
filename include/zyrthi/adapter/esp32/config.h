/**
 * @file config.h
 * @brief 硬件配置（默认引脚映射、实例配置）
 */
#ifndef ZYRTHI_ADAPTER_ESP32_CONFIG_H
#define ZYRTHI_ADAPTER_ESP32_CONFIG_H

#include "chip_config.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================
 * UART 默认配置
 * ========================== */
// 从 Kconfig 读取配置，若无则使用芯片默认值
#ifndef CONFIG_ZYRTHI_UART_DEFAULT_TX
    #if defined(ZYRTHI_CHIP_ESP32) || defined(ZYRTHI_CHIP_ESP32S3)
        #define CONFIG_ZYRTHI_UART_DEFAULT_TX   1
        #define CONFIG_ZYRTHI_UART_DEFAULT_RX   3
    #elif defined(ZYRTHI_CHIP_ESP32C3)
        #define CONFIG_ZYRTHI_UART_DEFAULT_TX   21
        #define CONFIG_ZYRTHI_UART_DEFAULT_RX   20
    #endif
    #define CONFIG_ZYRTHI_UART_DEFAULT_BAUD     115200
#endif

#define ZYRTHI_UART_DEFAULT_TX      CONFIG_ZYRTHI_UART_DEFAULT_TX
#define ZYRTHI_UART_DEFAULT_RX      CONFIG_ZYRTHI_UART_DEFAULT_RX
#define ZYRTHI_UART_DEFAULT_BAUD    CONFIG_ZYRTHI_UART_DEFAULT_BAUD

/* ==========================
 * I2C 默认配置
 * ========================== */
#ifndef CONFIG_ZYRTHI_I2C_DEFAULT_SDA
    #if defined(ZYRTHI_CHIP_ESP32) || defined(ZYRTHI_CHIP_ESP32S3)
        #define CONFIG_ZYRTHI_I2C_DEFAULT_SDA   21
        #define CONFIG_ZYRTHI_I2C_DEFAULT_SCL   22
    #elif defined(ZYRTHI_CHIP_ESP32C3)
        #define CONFIG_ZYRTHI_I2C_DEFAULT_SDA   6
        #define CONFIG_ZYRTHI_I2C_DEFAULT_SCL   7
    #endif
    #define CONFIG_ZYRTHI_I2C_DEFAULT_CLOCK     100000
#endif

#define ZYRTHI_I2C_DEFAULT_SDA      CONFIG_ZYRTHI_I2C_DEFAULT_SDA
#define ZYRTHI_I2C_DEFAULT_SCL      CONFIG_ZYRTHI_I2C_DEFAULT_SCL
#define ZYRTHI_I2C_DEFAULT_CLOCK    CONFIG_ZYRTHI_I2C_DEFAULT_CLOCK

/* ==========================
 * PWM 默认配置
 * ========================== */
#ifndef CONFIG_ZYRTHI_PWM_DEFAULT_FREQ
    #define CONFIG_ZYRTHI_PWM_DEFAULT_FREQ     5000
#endif

#define ZYRTHI_PWM_DEFAULT_FREQ     CONFIG_ZYRTHI_PWM_DEFAULT_FREQ

/* ==========================
 * ADC 默认配置
 * ========================== */
#define ZYRTHI_ADC_DEFAULT_ATTEN    3       // 11dB 衰减 (测量范围 0-3.3V)
#define ZYRTHI_ADC_DEFAULT_WIDTH    12      // 12 位分辨率

/* ==========================
 * 超时配置
 * ========================== */
#define ZYRTHI_UART_TIMEOUT_MS      1000
#define ZYRTHI_I2C_TIMEOUT_MS       1000

#ifdef __cplusplus
}
#endif

#endif // ZYRTHI_ADAPTER_ESP32_CONFIG_H
