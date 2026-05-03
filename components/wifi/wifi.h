#pragma once

#include "esp_err.h"

/**
 * @brief Initialize WiFi in Station mode and connect using provisioned secrets.
 */
void wifi_init_sta(void);

/**
 * @brief Post a payload to the cloud ingest endpoint.
 *
 * @param json_payload The raw JSON payload received from I2C or elsewhere.
 */
void wifi_post_to_cloud(const char *json_payload);
