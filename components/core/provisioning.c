#include "provisioning.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "PROVISIONING";

esp_err_t init_provisioning(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    // initialize the custom secrets partition
    ret = nvs_flash_init_partition("storage");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize secrets partition: %s",
                 esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t get_secret(const char *key, char *value, size_t max_len) {
    nvs_handle_t handle;

    esp_err_t err =
        nvs_open_from_partition("storage", "secrets", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening nvs partition 'storage': %s",
                 esp_err_to_name(err));
        return err;
    }

    err = nvs_get_str(handle, key, value, &max_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading secret '%s': %s", key,
                 esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}
