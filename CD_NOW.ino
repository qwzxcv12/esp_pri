#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "wifi_config_html.h"

static const char *TAG = "wifi_manager";

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
#define MAXIMUM_RETRY  3

// MQTT globals
static esp_mqtt_client_handle_t mqtt_client = NULL;
static char g_dev_id[128] = {0};
static char g_dev_key[128] = {0};
static char g_mqtt_topic[256] = {0};

// NVS Helper functions
esp_err_t save_wifi_credentials(const char* ssid, const char* password) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("wifi_creds", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_set_str(my_handle, "ssid", ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting SSID: %s", esp_err_to_name(err));
        nvs_close(my_handle);
        return err;
    }
    
    err = nvs_set_str(my_handle, "password", password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting password: %s", esp_err_to_name(err));
        nvs_close(my_handle);
        return err;
    }
    
    err = nvs_commit(my_handle);
    nvs_close(my_handle);
    return err;
}

esp_err_t read_wifi_credentials(char* ssid, size_t ssid_len, char* password, size_t pass_len) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("wifi_creds", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        return err;
    }
    
    err = nvs_get_str(my_handle, "ssid", ssid, &ssid_len);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return err;
    }
    
    size_t temp_len = pass_len;
    err = nvs_get_str(my_handle, "password", password, &temp_len);
    if (err != ESP_OK) {
        password[0] = '\0';
    }
    
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t save_mqtt_config(const char* server, const char* port, const char* user, const char* pass, const char* topic) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("mqtt_config", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    nvs_set_str(my_handle, "server", server);
    nvs_set_str(my_handle, "port", port);
    nvs_set_str(my_handle, "user", user);
    nvs_set_str(my_handle, "pass", pass);
    nvs_set_str(my_handle, "topic", topic);
    nvs_commit(my_handle);
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t read_mqtt_config(char* server, size_t server_len, char* port, size_t port_len, char* user, size_t user_len, char* pass, size_t pass_len, char* topic, size_t topic_len) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("mqtt_config", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        server[0] = '\0';
        port[0] = '\0';
        user[0] = '\0';
        pass[0] = '\0';
        topic[0] = '\0';
        return err;
    }
    if (nvs_get_str(my_handle, "server", server, &server_len) != ESP_OK) server[0] = '\0';
    if (nvs_get_str(my_handle, "port", port, &port_len) != ESP_OK) port[0] = '\0';
    if (nvs_get_str(my_handle, "user", user, &user_len) != ESP_OK) user[0] = '\0';
    if (nvs_get_str(my_handle, "pass", pass, &pass_len) != ESP_OK) pass[0] = '\0';
    if (nvs_get_str(my_handle, "topic", topic, &topic_len) != ESP_OK) topic[0] = '\0';
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t save_ws_config(const char* url) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("ws_config", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    nvs_set_str(my_handle, "url", url);
    nvs_commit(my_handle);
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t read_ws_config(char* url, size_t url_len) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("ws_config", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        url[0] = '\0';
        return err;
    }
    if (nvs_get_str(my_handle, "url", url, &url_len) != ESP_OK) url[0] = '\0';
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t save_dev_credentials(const char* dev_id, const char* dev_key) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("dev_creds", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    nvs_set_str(my_handle, "dev_id", dev_id);
    nvs_set_str(my_handle, "dev_key", dev_key);
    nvs_commit(my_handle);
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t read_dev_credentials(char* dev_id, size_t id_len, char* dev_key, size_t key_len) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("dev_creds", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        dev_id[0] = '\0';
        dev_key[0] = '\0';
        return err;
    }
    if (nvs_get_str(my_handle, "dev_id", dev_id, &id_len) != ESP_OK) dev_id[0] = '\0';
    if (nvs_get_str(my_handle, "dev_key", dev_key, &key_len) != ESP_OK) dev_key[0] = '\0';
    nvs_close(my_handle);
    return ESP_OK;
}

// ==================== MQTT CLIENT ====================
static const char *MQTT_TAG = "mqtt_qms";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "============================================");
        ESP_LOGI(MQTT_TAG, "MQTT CONNECTED to broker!");
        ESP_LOGI(MQTT_TAG, "============================================");
        // Subscribe to display command topic
        if (strlen(g_dev_id) > 0) {
            char sub_topic[256];
            snprintf(sub_topic, sizeof(sub_topic), "qms/display/%s/command", g_dev_id);
            int msg_id = esp_mqtt_client_subscribe(client, sub_topic, 1);
            ESP_LOGI(MQTT_TAG, "Subscribing to topic: %s (msg_id=%d)", sub_topic, msg_id);
        }
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(MQTT_TAG, "MQTT DISCONNECTED from broker. Will auto-reconnect...");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(MQTT_TAG, "MQTT PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(MQTT_TAG, "============================================");
        ESP_LOGI(MQTT_TAG, "MQTT DATA RECEIVED");
        ESP_LOGI(MQTT_TAG, "  Topic: %.*s", event->topic_len, event->topic);
        ESP_LOGI(MQTT_TAG, "  Payload: %.*s", event->data_len, event->data);
        ESP_LOGI(MQTT_TAG, "============================================");

        // Parse JSON
        {
            char *json_buf = (char*)malloc(event->data_len + 1);
            if (json_buf) {
                memcpy(json_buf, event->data, event->data_len);
                json_buf[event->data_len] = '\0';

                cJSON *root = cJSON_Parse(json_buf);
                if (root) {
                    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
                    if (cmd && cJSON_IsString(cmd)) {
                        if (strcmp(cmd->valuestring, "display_ticket") == 0) {
                            cJSON *data = cJSON_GetObjectItem(root, "data");
                            if (data) {
                                const char *ticket = cJSON_GetStringValue(cJSON_GetObjectItem(data, "ticket"));
                                const char *counter = cJSON_GetStringValue(cJSON_GetObjectItem(data, "counter"));
                                const char *service = cJSON_GetStringValue(cJSON_GetObjectItem(data, "service"));
                                const char *cust_name = cJSON_GetStringValue(cJSON_GetObjectItem(data, "cust_name"));
                                ESP_LOGI(MQTT_TAG, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
                                ESP_LOGI(MQTT_TAG, "  CMD: display_ticket");
                                ESP_LOGI(MQTT_TAG, "  TICKET : %s", ticket ? ticket : "N/A");
                                ESP_LOGI(MQTT_TAG, "  COUNTER: %s", counter ? counter : "N/A");
                                ESP_LOGI(MQTT_TAG, "  SERVICE: %s", service ? service : "N/A");
                                if (cust_name) {
                                    ESP_LOGI(MQTT_TAG, "  CUSTOMER: %s", cust_name);
                                }
                                ESP_LOGI(MQTT_TAG, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
                            }
                        } else if (strcmp(cmd->valuestring, "clear_display") == 0) {
                            ESP_LOGI(MQTT_TAG, "  CMD: clear_display - Clearing screen");
                        } else {
                            ESP_LOGI(MQTT_TAG, "  Unknown CMD: %s", cmd->valuestring);
                        }
                    }
                    cJSON_Delete(root);
                } else {
                    ESP_LOGW(MQTT_TAG, "Failed to parse JSON payload");
                }
                free(json_buf);
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(MQTT_TAG, "MQTT ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGE(MQTT_TAG, "  Transport error: 0x%x", event->error_handle->esp_transport_sock_errno);
        }
        break;

    default:
        ESP_LOGD(MQTT_TAG, "MQTT event id: %d", event->event_id);
        break;
    }
}

// Heartbeat task: publish heartbeat every 20s
static void mqtt_heartbeat_task(void *pvParameters)
{
    char hb_topic[256];
    snprintf(hb_topic, sizeof(hb_topic), "qms/heartbeat/%s", g_dev_id);

    char hb_payload[256];
    snprintf(hb_payload, sizeof(hb_payload), "{\"secret_key\":\"%s\"}", g_dev_key);

    ESP_LOGI(MQTT_TAG, "Heartbeat task started. Topic: %s", hb_topic);

    while (1) {
        if (mqtt_client) {
            int msg_id = esp_mqtt_client_publish(mqtt_client, hb_topic, hb_payload, 0, 0, 0);
            ESP_LOGI(MQTT_TAG, "Heartbeat sent (msg_id=%d)", msg_id);
        }
        vTaskDelay(pdMS_TO_TICKS(20000)); // 20 seconds
    }
}

void mqtt_app_start(const char* broker, int port, const char* user, const char* pass)
{
    ESP_LOGI(MQTT_TAG, "============================================");
    ESP_LOGI(MQTT_TAG, "Starting MQTT Client");
    ESP_LOGI(MQTT_TAG, "  Broker: %s:%d", broker, port);
    ESP_LOGI(MQTT_TAG, "  User  : %s", strlen(user) > 0 ? user : "[none]");
    ESP_LOGI(MQTT_TAG, "  Dev ID: %.16s...", g_dev_id);
    ESP_LOGI(MQTT_TAG, "============================================");

    char uri[256];
    snprintf(uri, sizeof(uri), "mqtt://%s:%d", broker, port);

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = uri;
    if (strlen(user) > 0) {
        mqtt_cfg.credentials.username = user;
    }
    if (strlen(pass) > 0) {
        mqtt_cfg.credentials.authentication.password = pass;
    }
    mqtt_cfg.network.reconnect_timeout_ms = 5000;

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    // Start heartbeat task
    if (strlen(g_dev_id) > 0 && strlen(g_dev_key) > 0) {
        xTaskCreate(mqtt_heartbeat_task, "mqtt_hb", 4096, NULL, 5, NULL);
    } else {
        ESP_LOGW(MQTT_TAG, "Device ID or KEY not set, heartbeat disabled.");
    }
}

// ==================== WIFI EVENT HANDLER ====================
// Event handler for wifi and IP events
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting to AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry connection to AP (%d/%d)", s_retry_num, MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Successfully got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Helper function to decode URL-encoded string
void url_decode(char *dst, const char *src) {
    char a = 0, b = 0;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit((unsigned char)a) && isxdigit((unsigned char)b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Web Server Handlers (html_page is imported from wifi_config_html.h)

// Helper function to replace placeholders in a string
std::string replace_placeholder(std::string str, const std::string& placeholder, const std::string& replacement) {
    size_t pos = 0;
    while ((pos = str.find(placeholder, pos)) != std::string::npos) {
        str.replace(pos, placeholder.length(), replacement);
        pos += replacement.length();
    }
    return str;
}

// Helper function to extract and decode a URL parameter
bool parse_url_param(const char* body, const char* key, char* dst, size_t dst_max) {
    char key_eq[64];
    snprintf(key_eq, sizeof(key_eq), "%s=", key);
    const char* ptr = strstr(body, key_eq);
    if (!ptr) {
        dst[0] = '\0';
        return false;
    }
    ptr += strlen(key_eq);
    const char* end = strchr(ptr, '&');
    size_t len = end ? (size_t)(end - ptr) : strlen(ptr);
    if (len >= dst_max) {
        len = dst_max - 1;
    }
    char raw[256];
    if (len >= sizeof(raw)) len = sizeof(raw) - 1;
    strncpy(raw, ptr, len);
    raw[len] = '\0';
    url_decode(dst, raw);
    return true;
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    char ssid[64] = {0};
    char password[64] = {0};
    char mqtt_server[64] = {0};
    char mqtt_port[16] = {0};
    char mqtt_user[64] = {0};
    char mqtt_pass[64] = {0};
    char mqtt_topic[64] = {0};
    char ws_url[128] = {0};
    char dev_id[128] = {0};
    char dev_key[128] = {0};

    // Read current config
    read_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password));
    read_mqtt_config(mqtt_server, sizeof(mqtt_server), mqtt_port, sizeof(mqtt_port), mqtt_user, sizeof(mqtt_user), mqtt_pass, sizeof(mqtt_pass), mqtt_topic, sizeof(mqtt_topic));
    read_ws_config(ws_url, sizeof(ws_url));
    read_dev_credentials(dev_id, sizeof(dev_id), dev_key, sizeof(dev_key));

    // Substitute placeholders
    std::string html = html_page;
    html = replace_placeholder(html, "{{SSID}}", ssid);
    html = replace_placeholder(html, "{{PASSWORD}}", password);
    html = replace_placeholder(html, "{{MQTT_SERVER}}", mqtt_server);
    html = replace_placeholder(html, "{{MQTT_PORT}}", strlen(mqtt_port) > 0 ? mqtt_port : "1883");
    html = replace_placeholder(html, "{{MQTT_USER}}", mqtt_user);
    html = replace_placeholder(html, "{{MQTT_PASS}}", mqtt_pass);
    html = replace_placeholder(html, "{{MQTT_TOPIC}}", mqtt_topic);
    html = replace_placeholder(html, "{{WS_URL}}", ws_url);
    html = replace_placeholder(html, "{{DEV_ID}}", dev_id);
    html = replace_placeholder(html, "{{DEV_KEY}}", dev_key);

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html.c_str(), html.length());
}

void restart_task(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Restarting ESP32 device...");
    esp_restart();
}

static esp_err_t config_post_handler(httpd_req_t *req)
{
    char buf[1024];
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Payload too long");
        return ESP_FAIL;
    }

    int cur_len = 0;
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf + cur_len, remaining)) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        cur_len += ret;
        remaining -= ret;
    }
    buf[cur_len] = '\0';

    char ssid[64] = {0};
    char password[64] = {0};
    char mqtt_server[64] = {0};
    char mqtt_port[16] = {0};
    char mqtt_user[64] = {0};
    char mqtt_pass[64] = {0};
    char mqtt_topic[64] = {0};
    char ws_url[128] = {0};
    char dev_id[128] = {0};
    char dev_key[128] = {0};

    parse_url_param(buf, "ssid", ssid, sizeof(ssid));
    parse_url_param(buf, "password", password, sizeof(password));
    parse_url_param(buf, "mqtt_server", mqtt_server, sizeof(mqtt_server));
    parse_url_param(buf, "mqtt_port", mqtt_port, sizeof(mqtt_port));
    parse_url_param(buf, "mqtt_user", mqtt_user, sizeof(mqtt_user));
    parse_url_param(buf, "mqtt_pass", mqtt_pass, sizeof(mqtt_pass));
    parse_url_param(buf, "mqtt_topic", mqtt_topic, sizeof(mqtt_topic));
    parse_url_param(buf, "ws_url", ws_url, sizeof(ws_url));
    parse_url_param(buf, "dev_id", dev_id, sizeof(dev_id));
    parse_url_param(buf, "dev_key", dev_key, sizeof(dev_key));

    ESP_LOGI(TAG, "Saving configurations...");

    esp_err_t err1 = save_wifi_credentials(ssid, password);
    esp_err_t err2 = save_mqtt_config(mqtt_server, mqtt_port, mqtt_user, mqtt_pass, mqtt_topic);
    esp_err_t err3 = save_ws_config(ws_url);
    esp_err_t err4 = save_dev_credentials(dev_id, dev_key);

    if (err1 != ESP_OK || err2 != ESP_OK || err3 != ESP_OK || err4 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save configuration to NVS!");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save configuration");
        return ESP_FAIL;
    }

    const char* success_html_1 = 
    "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<title>Success</title><style>"
    "body { font-family: sans-serif; background: #121214; color: #fff; text-align: center; padding-top: 50px; }"
    "h2 { color: #00adb5; }"
    "</style></head><body>"
    "<h2>Configuration Saved Successfully!</h2>"
    "<p>ESP32 will restart and apply the new configuration shortly.</p></body></html>";
    
    httpd_resp_sendstr_chunk(req, success_html_1);
    httpd_resp_sendstr_chunk(req, NULL);

    xTaskCreate(restart_task, "restart_task", 2048, NULL, 5, NULL);

    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting web server on port: %d", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t config_post = {
            .uri       = "/config",
            .method    = HTTP_POST,
            .handler   = config_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &config_post);
        return server;
    }

    ESP_LOGE(TAG, "Error starting web server!");
    return NULL;
}

extern "C" void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS Storage Initialized");

    // Initialize Network and Events
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create netif instances
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    (void)sta_netif;
    (void)ap_netif;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register Event Handler
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

    // Read WiFi, MQTT, and WebSocket configuration
    char ssid[64] = {0};
    char password[64] = {0};
    esp_err_t err = read_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password));

    char mqtt_server[64] = {0};
    char mqtt_port[16] = {0};
    char mqtt_user[64] = {0};
    char mqtt_pass[64] = {0};
    char mqtt_topic[64] = {0};
    char ws_url[128] = {0};
    char dev_id[128] = {0};
    char dev_key[128] = {0};
    read_mqtt_config(mqtt_server, sizeof(mqtt_server), mqtt_port, sizeof(mqtt_port), mqtt_user, sizeof(mqtt_user), mqtt_pass, sizeof(mqtt_pass), mqtt_topic, sizeof(mqtt_topic));
    read_ws_config(ws_url, sizeof(ws_url));
    read_dev_credentials(dev_id, sizeof(dev_id), dev_key, sizeof(dev_key));

    ESP_LOGI(TAG, "---------------------------------------------");
    ESP_LOGI(TAG, "Loaded Device Configurations from NVS:");
    ESP_LOGI(TAG, "  WiFi SSID      : %s", strlen(ssid) > 0 ? ssid : "[Not Set]");
    ESP_LOGI(TAG, "  MQTT Broker    : %s:%s", strlen(mqtt_server) > 0 ? mqtt_server : "[Not Set]", strlen(mqtt_port) > 0 ? mqtt_port : "[Not Set]");
    ESP_LOGI(TAG, "  MQTT Username  : %s", strlen(mqtt_user) > 0 ? mqtt_user : "[Not Set]");
    ESP_LOGI(TAG, "  MQTT Topic     : %s", strlen(mqtt_topic) > 0 ? mqtt_topic : "[Not Set]");
    ESP_LOGI(TAG, "  WebSocket URL  : %s", strlen(ws_url) > 0 ? ws_url : "[Not Set]");
    ESP_LOGI(TAG, "  Device ID      : %s", strlen(dev_id) > 0 ? dev_id : "[Not Set]");
    ESP_LOGI(TAG, "  Device KEY     : %s", strlen(dev_key) > 0 ? dev_key : "[Not Set]");
    ESP_LOGI(TAG, "---------------------------------------------");

    bool connected = false;
    if (err == ESP_OK && strlen(ssid) > 0) {
        ESP_LOGI(TAG, "Found stored WiFi credentials: SSID='%s'", ssid);
        
        s_wifi_event_group = xEventGroupCreate();
        s_retry_num = 0;

        wifi_config_t wifi_config = {};
        strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "Waiting for WiFi connection...");
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                pdFALSE,
                pdFALSE,
                portMAX_DELAY);

        if (bits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "Successfully connected to WiFi SSID: %s", ssid);
            connected = true;
        } else if (bits & WIFI_FAIL_BIT) {
            ESP_LOGW(TAG, "Failed to connect to WiFi SSID: %s after %d retries.", ssid, MAXIMUM_RETRY);
        }

        vEventGroupDelete(s_wifi_event_group);
    } else {
        ESP_LOGI(TAG, "No stored WiFi credentials found in NVS.");
    }

    if (!connected) {
        ESP_LOGI(TAG, "Starting Access Point and Captive Portal...");
        
        // Stop STA mode if it was started
        esp_wifi_stop();

        wifi_config_t wifi_config = {};
        const char* ap_ssid = "ESP32_WiFi_Config";
        strncpy((char*)wifi_config.ap.ssid, ap_ssid, sizeof(wifi_config.ap.ssid));
        wifi_config.ap.ssid_len = strlen(ap_ssid);
        wifi_config.ap.channel = 1;
        wifi_config.ap.max_connection = 4;
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "AP Mode started. SSID: %s", ap_ssid);
        ESP_LOGI(TAG, "Connect to the AP and open http://192.168.4.1 in your browser.");

        start_webserver();
    } else {
        ESP_LOGI(TAG, "Starting Web Server in Station mode...");
        start_webserver();

        // Start MQTT client if broker is configured
        if (strlen(mqtt_server) > 0) {
            // Copy dev credentials to globals for MQTT tasks
            strncpy(g_dev_id, dev_id, sizeof(g_dev_id) - 1);
            strncpy(g_dev_key, dev_key, sizeof(g_dev_key) - 1);

            int port = 1883;
            if (strlen(mqtt_port) > 0) {
                port = atoi(mqtt_port);
            }
            mqtt_app_start(mqtt_server, port, mqtt_user, mqtt_pass);
        } else {
            ESP_LOGW(TAG, "MQTT Broker not configured. Skipping MQTT connection.");
        }
    }
}
