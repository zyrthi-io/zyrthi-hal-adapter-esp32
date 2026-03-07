/**
 * @file adc_map.h
 * @brief ADC 引脚到通道映射（ESP32 / ESP32-S3 / ESP32-C3）
 */
#ifndef ZYRTHI_ADAPTER_ESP32_ADC_MAP_H
#define ZYRTHI_ADAPTER_ESP32_ADC_MAP_H

#include "chip_config.h"
#include "esp_adc/adc_oneshot.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================
 * GPIO -> ADC 单元/通道映射
 * 返回值: 0=成功, -1=无效引脚
 * ========================== */

/**
 * @brief 将 GPIO 编号转换为 ADC 单元和通道
 * @param gpio: GPIO 编号
 * @param unit: 输出 ADC 单元 (ADC_UNIT_1 或 ADC_UNIT_2)
 * @param channel: 输出 ADC 通道
 * @return 0 成功, -1 无效引脚
 */
static inline int zyrthi_adc_gpio_to_channel(int gpio, adc_unit_t *unit, adc_channel_t *channel) {
#if defined(ZYRTHI_CHIP_ESP32)
    // ESP32 ADC1 通道映射
    if (gpio >= 36 && gpio <= 39) {
        *unit = ADC_UNIT_1;
        *channel = (adc_channel_t)(gpio - 36);  // GPIO36=CH0, GPIO37=CH1, GPIO38=CH2, GPIO39=CH3
        return 0;
    }
    // ESP32 ADC1: GPIO32-35
    if (gpio >= 32 && gpio <= 35) {
        *unit = ADC_UNIT_1;
        *channel = (adc_channel_t)(gpio - 28);  // GPIO32=CH4, GPIO33=CH5, GPIO34=CH6, GPIO35=CH7
        return 0;
    }
    // ESP32 ADC2: GPIO4, GPIO0, GPIO2, GPIO15, GPIO13, GPIO12, GPIO14, GPIO27, GPIO25, GPIO26
    const int adc2_gpios[] = {4, 0, 2, 15, 13, 12, 14, 27, 25, 26};
    for (int i = 0; i < 10; i++) {
        if (gpio == adc2_gpios[i]) {
            *unit = ADC_UNIT_2;
            *channel = (adc_channel_t)i;
            return 0;
        }
    }

#elif defined(ZYRTHI_CHIP_ESP32S3)
    // ESP32-S3 ADC1: GPIO1-10
    if (gpio >= 1 && gpio <= 10) {
        *unit = ADC_UNIT_1;
        *channel = (adc_channel_t)(gpio - 1);
        return 0;
    }
    // ESP32-S3 ADC2: GPIO11-20
    if (gpio >= 11 && gpio <= 20) {
        *unit = ADC_UNIT_2;
        *channel = (adc_channel_t)(gpio - 11);
        return 0;
    }

#elif defined(ZYRTHI_CHIP_ESP32C3)
    // ESP32-C3 ADC1: GPIO0-4
    if (gpio >= 0 && gpio <= 4) {
        *unit = ADC_UNIT_1;
        *channel = (adc_channel_t)gpio;
        return 0;
    }
    // ESP32-C3 ADC2: GPIO5
    if (gpio == 5) {
        *unit = ADC_UNIT_2;
        *channel = ADC_CHANNEL_0;
        return 0;
    }
#endif

    (void)unit;
    (void)channel;
    return -1;  // 无效引脚
}

/**
 * @brief 检查 GPIO 是否支持 ADC 功能
 * @param gpio: GPIO 编号
 * @return 1 支持, 0 不支持
 */
static inline int zyrthi_adc_gpio_is_valid(int gpio) {
    adc_unit_t unit;
    adc_channel_t channel;
    return zyrthi_adc_gpio_to_channel(gpio, &unit, &channel) == 0;
}

#ifdef __cplusplus
}
#endif

#endif // ZYRTHI_ADAPTER_ESP32_ADC_MAP_H
