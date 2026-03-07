/**
 * @file chip_config.h
 * @brief 芯片差异配置（ESP32 / ESP32-S3 / ESP32-C3）
 */
#ifndef ZYRTHI_ADAPTER_ESP32_CHIP_CONFIG_H
#define ZYRTHI_ADAPTER_ESP32_CHIP_CONFIG_H

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================
 * 芯片识别宏
 * ========================== */
#if CONFIG_IDF_TARGET_ESP32
    #define ZYRTHI_CHIP_ESP32      1
    #define ZYRTHI_CHIP_NAME       "ESP32"
#elif CONFIG_IDF_TARGET_ESP32S3
    #define ZYRTHI_CHIP_ESP32S3    1
    #define ZYRTHI_CHIP_NAME       "ESP32-S3"
#elif CONFIG_IDF_TARGET_ESP32C3
    #define ZYRTHI_CHIP_ESP32C3    1
    #define ZYRTHI_CHIP_NAME       "ESP32-C3"
#else
    #error "Unsupported chip target"
#endif

/* ==========================
 * UART 配置
 * ========================== */
#if defined(ZYRTHI_CHIP_ESP32) || defined(ZYRTHI_CHIP_ESP32S3)
    #define ZYRTHI_UART_MAX_INSTANCES   3   // UART0, UART1, UART2
#else // ESP32-C3
    #define ZYRTHI_UART_MAX_INSTANCES   2   // UART0, UART1
#endif

/* ==========================
 * I2C 配置
 * ========================== */
#if defined(ZYRTHI_CHIP_ESP32) || defined(ZYRTHI_CHIP_ESP32S3)
    #define ZYRTHI_I2C_MAX_INSTANCES    2   // I2C0, I2C1
#else // ESP32-C3
    #define ZYRTHI_I2C_MAX_INSTANCES    1   // I2C0 only
#endif

/* ==========================
 * ADC 配置
 * ========================== */
#define ZYRTHI_ADC_MAX_UNITS         2   // ADC1, ADC2 (所有芯片相同)

// ADC 通道数因芯片而异
#if defined(ZYRTHI_CHIP_ESP32)
    #define ZYRTHI_ADC1_MAX_CHANNELS  8
    #define ZYRTHI_ADC2_MAX_CHANNELS  10
#elif defined(ZYRTHI_CHIP_ESP32S3)
    #define ZYRTHI_ADC1_MAX_CHANNELS  10
    #define ZYRTHI_ADC2_MAX_CHANNELS  10
#elif defined(ZYRTHI_CHIP_ESP32C3)
    #define ZYRTHI_ADC1_MAX_CHANNELS  5
    #define ZYRTHI_ADC2_MAX_CHANNELS  5
#endif

/* ==========================
 * PWM (LEDC) 配置
 * ========================== */
#if defined(ZYRTHI_CHIP_ESP32)
    #define ZYRTHI_PWM_MAX_CHANNELS    16
    #define ZYRTHI_PWM_MAX_TIMERS      4
#elif defined(ZYRTHI_CHIP_ESP32S3)
    #define ZYRTHI_PWM_MAX_CHANNELS    8
    #define ZYRTHI_PWM_MAX_TIMERS      4
#elif defined(ZYRTHI_CHIP_ESP32C3)
    #define ZYRTHI_PWM_MAX_CHANNELS    6
    #define ZYRTHI_PWM_MAX_TIMERS      4
#endif

/* ==========================
 * GPIO 配置
 * ========================== */
#if defined(ZYRTHI_CHIP_ESP32)
    #define ZYRTHI_GPIO_MAX_PIN        39
#elif defined(ZYRTHI_CHIP_ESP32S3)
    #define ZYRTHI_GPIO_MAX_PIN        48
#elif defined(ZYRTHI_CHIP_ESP32C3)
    #define ZYRTHI_GPIO_MAX_PIN        21
#endif

#ifdef __cplusplus
}
#endif

#endif // ZYRTHI_ADAPTER_ESP32_CHIP_CONFIG_H
