/**
 * @file adc.c
 * @brief ADC 模块实现（ESP-IDF v5.x oneshot 驱动）
 */

#include "zyrthi/adapter/esp32/config.h"
#include "zyrthi/hal/def.h"
#include "zyrthi/adapter/esp32/adc_map.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "zyrthi.adc";

typedef struct { adc_oneshot_unit_handle_t h; adc_cali_handle_t cali; bool init; } adc_inst_t;
static adc_inst_t adc_units[ZYRTHI_ADC_MAX_UNITS];

#include "zyrthi/hal/adc.h"

static adc_status_t ensure_unit(adc_unit_t unit) {
    if (adc_units[unit].init) return ADC_OK;
    adc_oneshot_unit_init_cfg_t cfg = {
        .unit_id = unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    if (adc_oneshot_new_unit(&cfg, &adc_units[unit].h) != ESP_OK) return ADC_ERROR_UNSUPPORTED;
    
#if CONFIG_IDF_TARGET_ESP32
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = unit,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_line_fitting(&cali_cfg, &adc_units[unit].cali);
#else
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = unit,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_curve_fitting(&cali_cfg, &adc_units[unit].cali);
#endif
    adc_units[unit].init = true;
    ESP_LOGI(TAG, "ADC%d initialized", unit + 1);
    return ADC_OK;
}

adc_status_t adc_open(u8_t pin, adc_config_t cfg) {
    adc_unit_t unit; adc_channel_t ch;
    if (zyrthi_adc_gpio_to_channel(pin, &unit, &ch) != 0) { ESP_LOGE(TAG, "Invalid ADC pin %d", pin); return ADC_ERROR_INVALID_PIN; }
    if (ensure_unit(unit) != ADC_OK) return ADC_ERROR_UNSUPPORTED;

    adc_atten_t atten = ADC_ATTEN_DB_12;
    if (cfg.atten == 0) atten = ADC_ATTEN_DB_0;
    else if (cfg.atten == 1) atten = ADC_ATTEN_DB_2_5;
    else if (cfg.atten == 2) atten = ADC_ATTEN_DB_6;

    adc_oneshot_chan_cfg_t ch_cfg = {
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    if (adc_oneshot_config_channel(adc_units[unit].h, ch, &ch_cfg) != ESP_OK) return ADC_ERROR_UNSUPPORTED;
    ESP_LOGI(TAG, "ADC pin %d opened (atten=%d)", pin, cfg.atten);
    return ADC_OK;
}

adc_status_t adc_close(u8_t pin) {
    ESP_LOGI(TAG, "ADC pin %d closed", pin);
    return ADC_OK;
}

adc_result_t adc_read(u8_t pin) {
    adc_result_t r = {ADC_OK, 0, 0};
    adc_unit_t unit; adc_channel_t ch;
    if (zyrthi_adc_gpio_to_channel(pin, &unit, &ch) != 0) { r.status = ADC_ERROR_INVALID_PIN; return r; }
    if (ensure_unit(unit) != ADC_OK) { r.status = ADC_ERROR_UNSUPPORTED; return r; }

    int raw;
    if (adc_oneshot_read(adc_units[unit].h, ch, &raw) != ESP_OK) { r.status = ADC_ERROR_TIMEOUT; return r; }
    r.raw = (u16_t)raw;

    if (adc_units[unit].cali) {
        int mv;
        if (adc_cali_raw_to_voltage(adc_units[unit].cali, raw, &mv) == ESP_OK) r.volt_mv = (u16_t)mv;
        else r.volt_mv = (u16_t)((raw * 3300) / 4095);
    } else {
        r.volt_mv = (u16_t)((raw * 3300) / 4095);
    }
    return r;
}