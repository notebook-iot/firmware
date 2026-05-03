idf_component_register(SRCS "wifi.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES core esp_wifi nvs_flash esp_event lwip esp_netif esp_http_client json esp_timer mbedtls)
