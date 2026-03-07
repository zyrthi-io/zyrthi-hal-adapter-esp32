/**
 * @file i2c.c
 * @brief I2C 模块实现（ESP-IDF v5.x 新驱动）
 */

#include "zyrthi/adapter/esp32/config.h"
#include "zyrthi/hal/def.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "zyrthi.i2c";

struct i2c_inst {
    i2c_master_bus_handle_t bus_handle;
    i2c_port_t port;
    bool initialized;
    u32_t clock_hz;
    i2c_master_dev_handle_t current_dev;
    u8_t current_addr;
};
static struct i2c_inst i2c_instances[ZYRTHI_I2C_MAX_INSTANCES];

#include "zyrthi/hal/i2c.h"

i2c_hdl_t i2c_default(void) { return &i2c_instances[0]; }
i2c_hdl_t i2c_get(u8_t num) { return (num < ZYRTHI_I2C_MAX_INSTANCES) ? &i2c_instances[num] : NULL; }

i2c_status_t i2c_open(i2c_hdl_t hdl, i2c_config_t cfg) {
    if (!hdl) return I2C_ERROR_INVALID_ARG;
    if (hdl->initialized) return I2C_OK;

    gpio_num_t sda = cfg.sda_pin, scl = cfg.scl_pin;
    if (sda == 0 || scl == 0) { sda = ZYRTHI_I2C_DEFAULT_SDA; scl = ZYRTHI_I2C_DEFAULT_SCL; }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = hdl->port, .sda_io_num = sda, .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus;
    if (i2c_new_master_bus(&bus_cfg, &bus) != ESP_OK) return I2C_ERROR_INVALID_ARG;

    hdl->bus_handle = bus;
    hdl->initialized = true;
    hdl->clock_hz = cfg.clock_hz;
    hdl->current_dev = NULL;
    ESP_LOGI(TAG, "I2C opened: SDA=%d, SCL=%d, clock=%lu", sda, scl, (unsigned long)cfg.clock_hz);
    return I2C_OK;
}

i2c_status_t i2c_close(i2c_hdl_t hdl) {
    if (!hdl || !hdl->initialized) return I2C_ERROR_INVALID_ARG;
    if (hdl->current_dev) { i2c_master_bus_rm_device(hdl->current_dev); hdl->current_dev = NULL; }
    i2c_del_master_bus(hdl->bus_handle);
    hdl->initialized = false;
    ESP_LOGI(TAG, "I2C closed");
    return I2C_OK;
}

i2c_status_t i2c_begin(i2c_hdl_t hdl, u8_t addr) {
    if (!hdl || !hdl->initialized) return I2C_ERROR_INVALID_ARG;
    if (hdl->current_dev && hdl->current_addr == addr) return I2C_OK;
    if (hdl->current_dev) { i2c_master_bus_rm_device(hdl->current_dev); hdl->current_dev = NULL; }

    i2c_device_config_t dev_cfg = { .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr, .scl_speed_hz = hdl->clock_hz };

    i2c_master_dev_handle_t dev;
    if (i2c_master_bus_add_device(hdl->bus_handle, &dev_cfg, &dev) != ESP_OK) return I2C_ERROR_INVALID_ARG;
    hdl->current_dev = dev;
    hdl->current_addr = addr;
    return I2C_OK;
}

i2c_result_t i2c_write(i2c_hdl_t hdl, const u8_t* data, u32_t len) {
    i2c_result_t r = {.status = I2C_ERROR_INVALID_ARG};
    if (!hdl || !hdl->initialized || !hdl->current_dev || !data || !len) return r;
    esp_err_t ret = i2c_master_transmit(hdl->current_dev, data, len, pdMS_TO_TICKS(ZYRTHI_I2C_TIMEOUT_MS));
    r.status = (ret == ESP_OK) ? I2C_OK : (ret == ESP_ERR_TIMEOUT) ? I2C_ERROR_TIMEOUT : I2C_ERROR_NACK;
    r.actual_len = (ret == ESP_OK) ? len : 0;
    return r;
}

i2c_result_t i2c_read(i2c_hdl_t hdl, u8_t* buf, u32_t len) {
    i2c_result_t r = {.status = I2C_ERROR_INVALID_ARG};
    if (!hdl || !hdl->initialized || !hdl->current_dev || !buf || !len) return r;
    esp_err_t ret = i2c_master_receive(hdl->current_dev, buf, len, pdMS_TO_TICKS(ZYRTHI_I2C_TIMEOUT_MS));
    r.status = (ret == ESP_OK) ? I2C_OK : (ret == ESP_ERR_TIMEOUT) ? I2C_ERROR_TIMEOUT : I2C_ERROR_NACK;
    r.actual_len = (ret == ESP_OK) ? len : 0;
    return r;
}

i2c_status_t i2c_end(i2c_hdl_t hdl) { return (!hdl || !hdl->initialized) ? I2C_ERROR_INVALID_ARG : I2C_OK; }

void i2c_adapter_init(void) {
    for (int i = 0; i < ZYRTHI_I2C_MAX_INSTANCES; i++)
        i2c_instances[i] = (struct i2c_inst){.port = i};
}
