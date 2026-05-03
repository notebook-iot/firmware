#include "esp_log.h"
#include "i2c.h"
#include "provisioning.h"
#include "wifi.h"
#include <string.h>

static const char *TAG = "MAIN";

void on_i2c_data(const uint8_t *data, size_t len) {
    char *payload = malloc(len + 1);

    if (payload) {
        memcpy(payload, data, len);
        payload[len] = '\0';
        wifi_post_to_cloud(payload);
        free(payload);
    }
}

void app_main(void) {
    // initialize NVS and custom secrets partition
    ESP_ERROR_CHECK(init_provisioning());

    // initialize WiFi
    wifi_init_sta();

    // start I2C slave and provide the callback
    start_slave(on_i2c_data);
}
