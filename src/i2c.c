/**
 * @file i2c.c
 * @brief I2C 模块实现（基于 ESP-IDF v5.x 新驱动，多实例支持）
 */
#include "zyrthi/hal/i2c.h"
#include "zyrthi/adapter/esp32/config.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "zyrthi.i2c";

// I2C 总线上下文
typedef struct {
    i2c_master_bus_handle_t bus_handle;
    i2c_port_t port;
    bool initialized;
    u32_t clock_hz;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    // 当前事务状态
    i2c_master_dev_handle_t current_dev;
    u8_t current_addr;
} i2c_ctx_t;

// I2C 实例池
static i2c_ctx_t i2c_buses[ZYRTHI_I2C_MAX_INSTANCES];

i2c_hdl_t i2c_default(void) {
    // 返回 I2C0 索引 + 1（避免 NULL）
    return (i2c_hdl_t)(intptr_t)1;
}

i2c_status_t i2c_init(i2c_hdl_t hdl, u32_t clock_hz) {
    int idx = (int)(intptr_t)hdl - 1;
    
    if (idx < 0 || idx >= ZYRTHI_I2C_MAX_INSTANCES) {
        ESP_LOGE(TAG, "Invalid I2C handle");
        return I2C_ERROR_INVALID_ARG;
    }

    // 已经初始化过
    if (i2c_buses[idx].initialized) {
        ESP_LOGW(TAG, "I2C%d already initialized", idx);
        return I2C_OK;
    }

    // 确定引脚
    gpio_num_t sda_pin, scl_pin;
    i2c_port_t port;
    
    switch (idx) {
        case 0:
            sda_pin = ZYRTHI_I2C_DEFAULT_SDA;
            scl_pin = ZYRTHI_I2C_DEFAULT_SCL;
            port = I2C_NUM_0;
            break;
#if ZYRTHI_I2C_MAX_INSTANCES > 1
        case 1:
            sda_pin = 33;
            scl_pin = 32;
            port = I2C_NUM_1;
            break;
#endif
        default:
            return I2C_ERROR_INVALID_ARG;
    }

    // 配置 I2C 主机总线
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = port,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(ret));
        return I2C_ERROR_INVALID_ARG;
    }

    i2c_buses[idx].bus_handle = bus_handle;
    i2c_buses[idx].port = port;
    i2c_buses[idx].initialized = true;
    i2c_buses[idx].clock_hz = clock_hz;
    i2c_buses[idx].sda_pin = sda_pin;
    i2c_buses[idx].scl_pin = scl_pin;
    i2c_buses[idx].current_dev = NULL;
    i2c_buses[idx].current_addr = 0;

    ESP_LOGI(TAG, "I2C%d initialized: SDA=%d, SCL=%d, clock=%d Hz", 
             idx, sda_pin, scl_pin, clock_hz);
    return I2C_OK;
}

i2c_status_t i2c_begin(i2c_hdl_t hdl, u8_t addr) {
    int idx = (int)(intptr_t)hdl - 1;
    
    if (idx < 0 || idx >= ZYRTHI_I2C_MAX_INSTANCES || !i2c_buses[idx].initialized) {
        ESP_LOGE(TAG, "I2C not initialized");
        return I2C_ERROR_INVALID_ARG;
    }

    i2c_ctx_t *ctx = &i2c_buses[idx];

    // 如果已有设备句柄且地址相同，直接返回
    if (ctx->current_dev != NULL && ctx->current_addr == addr) {
        return I2C_OK;
    }

    // 删除旧的设备句柄
    if (ctx->current_dev != NULL) {
        i2c_master_bus_rm_device(ctx->current_dev);
        ctx->current_dev = NULL;
    }

    // 创建新设备句柄
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = ctx->clock_hz,
    };

    i2c_master_dev_handle_t dev_handle;
    esp_err_t ret = i2c_master_bus_add_device(ctx->bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(ret));
        return I2C_ERROR_INVALID_ARG;
    }

    ctx->current_dev = dev_handle;
    ctx->current_addr = addr;

    return I2C_OK;
}

i2c_result_t i2c_write(i2c_hdl_t hdl, const u8_t* data, u32_t len) {
    i2c_result_t result = { .status = I2C_ERROR_INVALID_ARG, .actual_len = 0 };

    int idx = (int)(intptr_t)hdl - 1;
    
    if (idx < 0 || idx >= ZYRTHI_I2C_MAX_INSTANCES || 
        !i2c_buses[idx].initialized || i2c_buses[idx].current_dev == NULL) {
        return result;
    }

    if (data == NULL || len == 0) {
        result.status = I2C_ERROR_INVALID_ARG;
        return result;
    }

    esp_err_t ret = i2c_master_transmit(
        i2c_buses[idx].current_dev, 
        data, 
        len, 
        pdMS_TO_TICKS(ZYRTHI_I2C_TIMEOUT_MS)
    );

    if (ret == ESP_OK) {
        result.status = I2C_OK;
        result.actual_len = len;
    } else if (ret == ESP_ERR_TIMEOUT) {
        result.status = I2C_ERROR_TIMEOUT;
    } else {
        result.status = I2C_ERROR_NACK;
    }

    return result;
}

i2c_result_t i2c_read(i2c_hdl_t hdl, u8_t* buf, u32_t len) {
    i2c_result_t result = { .status = I2C_ERROR_INVALID_ARG, .actual_len = 0 };

    int idx = (int)(intptr_t)hdl - 1;
    
    if (idx < 0 || idx >= ZYRTHI_I2C_MAX_INSTANCES || 
        !i2c_buses[idx].initialized || i2c_buses[idx].current_dev == NULL) {
        return result;
    }

    if (buf == NULL || len == 0) {
        result.status = I2C_ERROR_INVALID_ARG;
        return result;
    }

    esp_err_t ret = i2c_master_receive(
        i2c_buses[idx].current_dev, 
        buf, 
        len, 
        pdMS_TO_TICKS(ZYRTHI_I2C_TIMEOUT_MS)
    );

    if (ret == ESP_OK) {
        result.status = I2C_OK;
        result.actual_len = len;
    } else if (ret == ESP_ERR_TIMEOUT) {
        result.status = I2C_ERROR_TIMEOUT;
    } else {
        result.status = I2C_ERROR_NACK;
    }

    return result;
}

i2c_status_t i2c_end(i2c_hdl_t hdl) {
    int idx = (int)(intptr_t)hdl - 1;
    
    if (idx < 0 || idx >= ZYRTHI_I2C_MAX_INSTANCES || !i2c_buses[idx].initialized) {
        return I2C_ERROR_INVALID_ARG;
    }

    return I2C_OK;
}
