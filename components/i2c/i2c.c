#include "i2c.h"
#include "config.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static const char *TAG = "I2C_SLAVE";

void start_slave(void) {
    i2c_config_t conf_slave = {
        .mode = I2C_MODE_SLAVE,
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .slave.addr_10bit_en = 0,
        .slave.slave_addr = I2C_SLAVE_ADDR,
        .clk_flags = 0,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_SLAVE_PORT, &conf_slave));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_SLAVE_PORT, conf_slave.mode,
                                       RX_BUFFER_LEN, 0, 0));

    uint8_t *data_rd = (uint8_t *)malloc(RX_BUFFER_LEN);
    if (data_rd == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for RX buffer");
        return;
    }
    ESP_LOGI(TAG, "I2C slave initialized at addr 0x%02x", I2C_SLAVE_ADDR);

    while (true) {
        int n_read = i2c_slave_read_buffer(I2C_SLAVE_PORT, data_rd,
                                           RX_BUFFER_LEN, pdMS_TO_TICKS(1000));

        if (n_read > 0) {
            if (n_read < RX_BUFFER_LEN) {
                data_rd[n_read] = '\0';
            } else {
                data_rd[RX_BUFFER_LEN - 1] = '\0';
            }

            ESP_LOGI(TAG, "Received %d bytes: %s", n_read, (char *)data_rd);
        }
    }

    free(data_rd);
}
