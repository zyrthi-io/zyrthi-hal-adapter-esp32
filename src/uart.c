/**
 * @file uart.c
 * @brief UART 模块实现（多实例支持）
 */
#include "zyrthi/hal/uart.h"
#include "zyrthi/adapter/esp32/config.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "string.h"

static const char *TAG = "zyrthi.uart";

// UART 上下文结构
typedef struct {
    uart_port_t port;
    bool initialized;
    u32_t baud;
} uart_ctx_t;

// UART 实例池
static uart_ctx_t uart_ctxs[ZYRTHI_UART_MAX_INSTANCES];

// 端口号转换
static inline uart_port_t hdl_to_port(uart_hdl_t hdl) {
    return (uart_port_t)(intptr_t)hdl;
}

static inline uart_hdl_t port_to_hdl(uart_port_t port) {
    return (uart_hdl_t)(intptr_t)(port + 1);  // +1 避免 NULL
}

uart_hdl_t uart_default(void) {
    // 返回 UART0 句柄
    return port_to_hdl(UART_NUM_0);
}

uart_status_t uart_init(uart_hdl_t hdl, u32_t baud) {
    uart_port_t port = hdl_to_port(hdl);
    
    // 验证端口
    if (port < 0 || port >= ZYRTHI_UART_MAX_INSTANCES) {
        ESP_LOGE(TAG, "Invalid UART handle");
        return UART_ERROR_INVALID_ARG;
    }

    // 已经初始化过，先删除
    if (uart_ctxs[port].initialized) {
        uart_driver_delete(port);
    }

    // 配置 UART
    uart_config_t cfg = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_param_config(port, &cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        return UART_ERROR_INVALID_ARG;
    }

    // 设置引脚（使用默认引脚）
    int tx_pin, rx_pin;
    switch (port) {
        case UART_NUM_0:
            tx_pin = ZYRTHI_UART_DEFAULT_TX;
            rx_pin = ZYRTHI_UART_DEFAULT_RX;
            break;
        case UART_NUM_1:
            // UART1 默认引脚（可根据需要修改）
            tx_pin = 4;
            rx_pin = 5;
            break;
#if ZYRTHI_UART_MAX_INSTANCES > 2
        case UART_NUM_2:
            tx_pin = 17;
            rx_pin = 16;
            break;
#endif
        default:
            return UART_ERROR_INVALID_ARG;
    }

    ret = uart_set_pin(port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(ret));
        return UART_ERROR_INVALID_ARG;
    }

    // 安装驱动
    ret = uart_driver_install(port, 256, 256, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        return UART_ERROR_INVALID_ARG;
    }

    uart_ctxs[port].port = port;
    uart_ctxs[port].initialized = true;
    uart_ctxs[port].baud = baud;

    ESP_LOGI(TAG, "UART%d initialized at %d baud", port, baud);
    return UART_OK;
}

uart_status_t uart_putc(uart_hdl_t hdl, u8_t c) {
    uart_port_t port = hdl_to_port(hdl);
    
    if (port < 0 || port >= ZYRTHI_UART_MAX_INSTANCES || !uart_ctxs[port].initialized) {
        return UART_ERROR_INVALID_ARG;
    }

    int ret = uart_write_bytes(port, &c, 1);
    if (ret != 1) {
        return UART_ERROR_TIMEOUT;
    }

    return UART_OK;
}

uart_result_t uart_getc(uart_hdl_t hdl) {
    uart_port_t port = hdl_to_port(hdl);
    uart_result_t result = { .status = UART_ERROR_INVALID_ARG };

    if (port < 0 || port >= ZYRTHI_UART_MAX_INSTANCES || !uart_ctxs[port].initialized) {
        return result;
    }

    u8_t c;
    int ret = uart_read_bytes(port, &c, 1, pdMS_TO_TICKS(ZYRTHI_UART_TIMEOUT_MS));
    if (ret == 1) {
        result.status = UART_OK;
        result.data.ch = c;
    } else if (ret == 0) {
        result.status = UART_ERROR_TIMEOUT;
    } else {
        result.status = UART_ERROR_EMPTY;
    }

    return result;
}

uart_result_t uart_available(uart_hdl_t hdl) {
    uart_port_t port = hdl_to_port(hdl);
    uart_result_t result = { .status = UART_ERROR_INVALID_ARG };

    if (port < 0 || port >= ZYRTHI_UART_MAX_INSTANCES || !uart_ctxs[port].initialized) {
        return result;
    }

    size_t len;
    uart_get_buffered_data_len(port, &len);
    
    result.status = UART_OK;
    result.data.available = len > 0;

    return result;
}

uart_result_t uart_puts(uart_hdl_t hdl, const char_t* str) {
    uart_port_t port = hdl_to_port(hdl);
    uart_result_t result = { .status = UART_ERROR_INVALID_ARG };

    if (port < 0 || port >= ZYRTHI_UART_MAX_INSTANCES || !uart_ctxs[port].initialized) {
        return result;
    }

    if (str == NULL) {
        result.status = UART_ERROR_INVALID_ARG;
        return result;
    }

    u32_t len = strlen(str);
    int ret = uart_write_bytes(port, str, len);
    
    result.status = (ret == len) ? UART_OK : UART_ERROR_TIMEOUT;
    result.data.len = ret;

    return result;
}

uart_result_t uart_gets(uart_hdl_t hdl, char_t* buf, u32_t len) {
    uart_port_t port = hdl_to_port(hdl);
    uart_result_t result = { .status = UART_ERROR_INVALID_ARG };

    if (port < 0 || port >= ZYRTHI_UART_MAX_INSTANCES || !uart_ctxs[port].initialized) {
        return result;
    }

    if (buf == NULL || len == 0) {
        result.status = UART_ERROR_INVALID_ARG;
        return result;
    }

    u32_t idx = 0;
    while (idx < len - 1) {
        u8_t c;
        int ret = uart_read_bytes(port, &c, 1, pdMS_TO_TICKS(ZYRTHI_UART_TIMEOUT_MS));
        if (ret != 1) {
            break;
        }
        if (c == '\n' || c == '\r') {
            break;
        }
        buf[idx++] = c;
    }
    buf[idx] = '\0';

    result.status = (idx > 0) ? UART_OK : UART_ERROR_EMPTY;
    result.data.len = idx;

    return result;
}
