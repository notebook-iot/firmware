#include "esp_log.h"
#include "i2c.h"
#include "provisioning.h"

static const char *TAG = "MAIN";

void app_main(void) {
    // initialize NVS and custom secrets partition
    ESP_ERROR_CHECK(init_provisioning());

    char api_key[64];
    if (get_secret("api_key", api_key, sizeof(api_key)) == ESP_OK) {
        ESP_LOGI(TAG, "Provisioned API Key found: %s", api_key);
    } else {
        ESP_LOGW(TAG, "No API Key found in provisioning partition.");
    }

    start_slave();
}
