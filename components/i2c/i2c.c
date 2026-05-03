#include "i2c.h"
#include "config.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "I2C_SLAVE";

static i2c_data_callback_t s_callback = NULL;

static void i2c_slave_task(void *arg) {
    ESP_LOGI(TAG, "I2C slave task started");
    
    // Buffer to accumulate fragmented data
    uint8_t *msg_buf = (uint8_t *)malloc(RX_BUFFER_LEN);
    uint8_t *temp_rd = (uint8_t *)malloc(RX_BUFFER_LEN);
    size_t msg_len = 0;

    if (msg_buf == NULL || temp_rd == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for RX buffers");
        vTaskDelete(NULL);
        return;
    }

    int loop_count = 0;
    while (true) {
        int n_read = i2c_slave_read_buffer(I2C_SLAVE_PORT, temp_rd,
                                           RX_BUFFER_LEN, pdMS_TO_TICKS(1000));

        if (n_read > 0) {
            ESP_LOGD(TAG, "Read %d bytes from hardware", n_read);
            
            for (int i = 0; i < n_read; i++) {
                if (temp_rd[i] == '\0') {
                    // End of message reached
                    if (msg_len > 0) {
                        ESP_LOGI(TAG, "Full message received (%d bytes)", (int)msg_len);
                        if (s_callback) {
                            s_callback(msg_buf, msg_len);
                        }
                        msg_len = 0; // Reset for next message
                    }
                } else {
                    if (msg_len < RX_BUFFER_LEN - 1) {
                        msg_buf[msg_len++] = temp_rd[i];
                    } else {
                        ESP_LOGW(TAG, "Message buffer overflow, discarding data");
                        msg_len = 0;
                    }
                }
            }
        } else if (n_read < 0) {
            ESP_LOGE(TAG, "I2C read error: %d", n_read);
        } else {
            if (++loop_count >= 30) {
                ESP_LOGI(TAG, "I2C slave task heartbeat (waiting for data...)");
                loop_count = 0;
            }
        }
    }

    free(msg_buf);
    free(temp_rd);
    vTaskDelete(NULL);
}

void start_slave(i2c_data_callback_t callback) {
    s_callback = callback;

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

    ESP_LOGI(TAG, "I2C slave initialized at addr 0x%02x", I2C_SLAVE_ADDR);
    xTaskCreate(i2c_slave_task, "i2c_slave_task", 4096, NULL, 5, NULL);
}
