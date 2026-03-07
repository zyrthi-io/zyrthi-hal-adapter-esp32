/**
 * @file uart.c
 * @brief UART 模块实现（多实例支持）
 */

#include "zyrthi/adapter/esp32/config.h"
#include "zyrthi/hal/uart.h"

/* ============================================================
 * ESP-IDF 类型冲突解决
 * HAL 已定义 uart_config_t，ESP-IDF 也有同名类型
 * 通过宏将 ESP-IDF 的 uart_config_t 重命名为 esp_uart_config_t
 * ============================================================ */
#define uart_config_t esp_uart_config_t
#include "driver/uart.h"
#undef uart_config_t
/* ============================================================ */

#include "esp_log.h"
#include "string.h"

static const char *TAG = "zyrthi.uart";

struct uart_inst { uart_port_t port; bool initialized; u32_t baud; };
static struct uart_inst uart_instances[ZYRTHI_UART_MAX_INSTANCES];

uart_hdl_t uart_default(void) { return &uart_instances[0]; }
uart_hdl_t uart_get(u8_t num) { return (num < ZYRTHI_UART_MAX_INSTANCES) ? &uart_instances[num] : NULL; }

uart_status_t uart_open(uart_hdl_t hdl, uart_config_t cfg) {
    if (!hdl) return UART_ERROR_INVALID_ARG;
    uart_port_t port = hdl->port;
    if (hdl->initialized) uart_driver_delete(port);

    u8_t db = cfg.data_bits ? cfg.data_bits : 8;
    u8_t sb = cfg.stop_bits ? cfg.stop_bits : 1;

    uart_word_length_t db_val = UART_DATA_8_BITS;
    if (db == 5) db_val = UART_DATA_5_BITS;
    else if (db == 6) db_val = UART_DATA_6_BITS;
    else if (db == 7) db_val = UART_DATA_7_BITS;

    uart_stop_bits_t sb_val = (sb == 2) ? UART_STOP_BITS_2 : UART_STOP_BITS_1;
    uart_parity_t p_val = UART_PARITY_DISABLE;
    if (cfg.parity == 1) p_val = UART_PARITY_ODD;
    else if (cfg.parity == 2) p_val = UART_PARITY_EVEN;

    // 使用 ESP-IDF 类型调用其 API
    esp_uart_config_t esp_cfg = {
        .baud_rate = cfg.baud,
        .data_bits = db_val,
        .parity = p_val,
        .stop_bits = sb_val,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    if (uart_param_config(port, &esp_cfg) != ESP_OK) return UART_ERROR_INVALID_ARG;

    int tx = cfg.tx_pin, rx = cfg.rx_pin;
    if (tx == 0 || rx == 0) {
        if (port == 0) { if (!tx) tx = ZYRTHI_UART_DEFAULT_TX; if (!rx) rx = ZYRTHI_UART_DEFAULT_RX; }
        else if (port == 1) { if (!tx) tx = 4; if (!rx) rx = 5; }
#if ZYRTHI_UART_MAX_INSTANCES > 2
        else if (port == 2) { if (!tx) tx = 17; if (!rx) rx = 16; }
#endif
        else return UART_ERROR_INVALID_ARG;
    }

    if (uart_set_pin(port, tx, rx, -1, -1) != ESP_OK) return UART_ERROR_INVALID_ARG;
    if (uart_driver_install(port, 256, 256, 0, NULL, 0) != ESP_OK) return UART_ERROR_INVALID_ARG;

    hdl->initialized = true;
    hdl->baud = cfg.baud;
    ESP_LOGI(TAG, "UART%d opened: baud=%lu, TX=%d, RX=%d", port, (unsigned long)cfg.baud, tx, rx);
    return UART_OK;
}

uart_status_t uart_close(uart_hdl_t hdl) {
    if (!hdl || !hdl->initialized) return UART_ERROR_INVALID_ARG;
    uart_driver_delete(hdl->port);
    hdl->initialized = false;
    ESP_LOGI(TAG, "UART%d closed", hdl->port);
    return UART_OK;
}

uart_status_t uart_putc(uart_hdl_t hdl, u8_t c) {
    if (!hdl || !hdl->initialized) return UART_ERROR_INVALID_ARG;
    return (uart_write_bytes(hdl->port, &c, 1) == 1) ? UART_OK : UART_ERROR_TIMEOUT;
}

uart_result_t uart_getc(uart_hdl_t hdl) {
    uart_result_t r = {.status = UART_ERROR_INVALID_ARG};
    if (!hdl || !hdl->initialized) return r;
    u8_t c;
    int ret = uart_read_bytes(hdl->port, &c, 1, pdMS_TO_TICKS(ZYRTHI_UART_TIMEOUT_MS));
    if (ret == 1) { r.status = UART_OK; r.data.ch = c; }
    else r.status = (ret == 0) ? UART_ERROR_TIMEOUT : UART_ERROR_EMPTY;
    return r;
}

uart_result_t uart_available(uart_hdl_t hdl) {
    uart_result_t r = {.status = UART_ERROR_INVALID_ARG};
    if (!hdl || !hdl->initialized) return r;
    size_t len;
    uart_get_buffered_data_len(hdl->port, &len);
    r.status = UART_OK; r.data.available = len > 0;
    return r;
}

uart_result_t uart_puts(uart_hdl_t hdl, const char_t* str) {
    uart_result_t r = {.status = UART_ERROR_INVALID_ARG};
    if (!hdl || !hdl->initialized || !str) return r;
    u32_t len = strlen(str);
    int ret = uart_write_bytes(hdl->port, str, len);
    r.status = (ret == (int)len) ? UART_OK : UART_ERROR_TIMEOUT;
    r.data.len = ret;
    return r;
}

uart_result_t uart_gets(uart_hdl_t hdl, char_t* buf, u32_t len) {
    uart_result_t r = {.status = UART_ERROR_INVALID_ARG};
    if (!hdl || !hdl->initialized || !buf || !len) return r;
    u32_t i = 0;
    while (i < len - 1) {
        u8_t c;
        if (uart_read_bytes(hdl->port, &c, 1, pdMS_TO_TICKS(ZYRTHI_UART_TIMEOUT_MS)) != 1) break;
        if (c == '\n' || c == '\r') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    r.status = i ? UART_OK : UART_ERROR_EMPTY;
    r.data.len = i;
    return r;
}

void uart_adapter_init(void) {
    for (int i = 0; i < ZYRTHI_UART_MAX_INSTANCES; i++)
        uart_instances[i] = (struct uart_inst){.port = i};
}
