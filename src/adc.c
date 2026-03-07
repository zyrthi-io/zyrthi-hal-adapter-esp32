/**
 * @file adc.c
 * @brief ADC 模块实现（基于 ESP-IDF v5.x oneshot 驱动，多实例支持）
 */
#include "zyrthi/hal/adc.h"
#include "zyrthi/adapter/esp32/config.h"
#include "zyrthi/adapter/esp32/adc_map.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "zyrthi.adc";

// ADC 单元上下文
typedef struct {
    adc_oneshot_unit_handle_t handle;
    adc_cali_handle_t cali_handle;
    bool initialized;
} adc_unit_ctx_t;

// ADC 单元实例池
static adc_unit_ctx_t adc_units[ZYRTHI_ADC_MAX_UNITS];

// 初始化指定 ADC 单元
static adc_status_t ensure_adc_unit(adc_unit_t unit) {
    if (adc_units[unit].initialized) {
        return ADC_OK;
    }

    // 创建 ADC 单元句柄
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t ret = adc_oneshot_new_unit(&init_cfg, &adc_units[unit].handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        return ADC_ERROR_UNSUPPORTED;
    }

    // 尝试创建校准句柄（可选，用于电压转换）
#if CONFIG_IDF_TARGET_ESP32
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = unit,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_cfg, &adc_units[unit].cali_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC calibration not available: %s", esp_err_to_name(ret));
        adc_units[unit].cali_handle = NULL;
    }
#else
    // ESP32-S3/C3 使用不同的校准方案
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = unit,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &adc_units[unit].cali_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC calibration not available: %s", esp_err_to_name(ret));
        adc_units[unit].cali_handle = NULL;
    }
#endif

    adc_units[unit].initialized = true;
    ESP_LOGI(TAG, "ADC%d initialized", unit + 1);
    return ADC_OK;
}

adc_result_t adc_read(u8_t pin) {
    adc_result_t result = { .status = ADC_OK, .raw = 0, .volt_mv = 0 };

    // GPIO -> ADC 单元/通道
    adc_unit_t unit;
    adc_channel_t channel;
    if (zyrthi_adc_gpio_to_channel(pin, &unit, &channel) != 0) {
        ESP_LOGE(TAG, "Pin %d is not a valid ADC pin", pin);
        result.status = ADC_ERROR_INVALID_PIN;
        return result;
    }

    // 确保 ADC 单元已初始化
    adc_status_t status = ensure_adc_unit(unit);
    if (status != ADC_OK) {
        result.status = status;
        return result;
    }

    // 配置通道（每次读取都配置，确保正确）
    adc_oneshot_chan_cfg_t ch_cfg = {
        .atten = ADC_ATTEN_DB_12,  // 11dB 衰减，测量范围 0-3.3V
        .bitwidth = ADC_BITWIDTH_12,
    };

    esp_err_t ret = adc_oneshot_config_channel(adc_units[unit].handle, channel, &ch_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(ret));
        result.status = ADC_ERROR_UNSUPPORTED;
        return result;
    }

    // 读取原始值
    int raw;
    ret = adc_oneshot_read(adc_units[unit].handle, channel, &raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_read failed: %s", esp_err_to_name(ret));
        result.status = ADC_ERROR_TIMEOUT;
        return result;
    }

    result.raw = (u16_t)raw;

    // 转换为电压（如果有校准）
    if (adc_units[unit].cali_handle != NULL) {
        int voltage;
        ret = adc_cali_raw_to_voltage(adc_units[unit].cali_handle, raw, &voltage);
        if (ret == ESP_OK) {
            result.volt_mv = (u16_t)voltage;
        } else {
            // 校准失败，使用公式估算
            result.volt_mv = (u16_t)((raw * 3300) / 4095);
        }
    } else {
        // 无校准，使用公式估算 (12-bit, 3.3V 参考电压)
        result.volt_mv = (u16_t)((raw * 3300) / 4095);
    }

    return result;
}
