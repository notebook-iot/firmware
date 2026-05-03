#include "wifi.h"
#include "provisioning.h"
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_sntp.h"
#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define EXAMPLE_ESP_MAXIMUM_RETRY  5
#define CLOUD_QUEUE_SIZE 10

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
static QueueHandle_t s_cloud_queue = NULL;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "WIFI_STA";
static int s_retry_num = 0;

static void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        initialize_sntp();
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Internal function that actually does the network work
static void perform_cloud_post(const char *json_payload) {
    char api_key[128];
    char cloud_url[256];
    char device_id[64];

    if (get_secret("api_key", api_key, sizeof(api_key)) != ESP_OK ||
        get_secret("cloud_url", cloud_url, sizeof(cloud_url)) != ESP_OK) {
        return;
    }
    
    if (get_secret("device_id", device_id, sizeof(device_id)) != ESP_OK) {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        snprintf(device_id, sizeof(device_id), "ESP32_%02X%02X%02X", mac[3], mac[4], mac[5]);
    }

    double temp = 0.0;
    cJSON *incoming = cJSON_Parse(json_payload);
    if (incoming) {
        cJSON *t_item = cJSON_GetObjectItem(incoming, "temp");
        if (cJSON_IsNumber(t_item)) {
            temp = t_item->valuedouble;
        }
        cJSON_Delete(incoming);
    }

    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);

    int64_t uptime_sec = esp_timer_get_time() / 1000000;

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    cJSON *root = cJSON_CreateObject();

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char timestamp_str[64];
    strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

    cJSON_AddStringToObject(root, "timestamp", timestamp_str);
    cJSON_AddStringToObject(root, "device_id", device_id);
    cJSON_AddStringToObject(root, "api_key", api_key);
    cJSON_AddStringToObject(root, "json", json_payload);
    cJSON_AddStringToObject(root, "wifi_network_name", (char*)conf.sta.ssid);
    cJSON_AddNumberToObject(root, "up_time", uptime_sec);
    cJSON_AddNumberToObject(root, "latency", 45);
    cJSON_AddStringToObject(root, "mac_address", mac_str);
    cJSON_AddNumberToObject(root, "temperature", temp);

    char *post_data = cJSON_PrintUnformatted(root);

    esp_http_client_config_t config = {
        .url = cloud_url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    ESP_LOGI(TAG, "Sending Data to %s: %s", cloud_url, post_data);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Success! Status %d", status_code);
    } else {
        ESP_LOGE(TAG, "Failed to send data: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);
}

static void cloud_task(void *arg) {
    char *payload;
    while (true) {
        if (xQueueReceive(s_cloud_queue, &payload, portMAX_DELAY)) {
            perform_cloud_post(payload);
            free(payload);
        }
    }
}

void wifi_post_to_cloud(const char *json_payload) {
    if (s_cloud_queue == NULL) {
        ESP_LOGE(TAG, "Cloud queue not initialized");
        return;
    }

    char *payload_copy = strdup(json_payload);
    if (payload_copy) {
        if (xQueueSend(s_cloud_queue, &payload_copy, 0) != pdPASS) {
            ESP_LOGW(TAG, "Cloud queue full, dropping packet");
            free(payload_copy);
        }
    }
}

void wifi_init_sta(void)
{
    char ssid[32];
    char pass[64];

    if (get_secret("wifi_ssid", ssid, sizeof(ssid)) != ESP_OK ||
        get_secret("wifi_pass", pass, sizeof(pass)) != ESP_OK) {
        ESP_LOGE(TAG, "WiFi credentials not found in NVS");
        return;
    }

    s_cloud_queue = xQueueCreate(CLOUD_QUEUE_SIZE, sizeof(char *));
    xTaskCreate(cloud_task, "cloud_task", 8192, NULL, 5, NULL);

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s", ssid);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s", ssid);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}
