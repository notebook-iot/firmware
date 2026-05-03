#pragma once
#include "esp_err.h"
#include <stddef.h>

/**
 * @brief Initialize NVS and the secrets partition.
 * @return ESP_OK on success
 */
esp_err_t init_provisioning(void);

/**
 * @brief Retrieve a secret from the provisioned storage.
 *
 * @param key The key to look up (e.g., "wifi_ssid")
 * @param value Buffer to store the result
 * @param max_len Size of the buffer
 * @return ESP_OK on success
 */
esp_err_t get_secret(const char *key, char *value, size_t max_len);
