#include <stdio.h>
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

static const char *TAG = "wifi_manager";

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
#define MAXIMUM_RETRY  3

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

// Web Server Handlers
static const char* html_page = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"    <title>ESP32 WiFi Configuration</title>"
"    <style>"
"        body {"
"            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;"
"            background: linear-gradient(135deg, #1e1e2f 0%, #111119 100%);"
"            color: #f0f0f0;"
"            display: flex;"
"            justify-content: center;"
"            align-items: center;"
"            height: 100vh;"
"            margin: 0;"
"        }"
"        .container {"
"            background: rgba(255, 255, 255, 0.05);"
"            backdrop-filter: blur(10px);"
"            border: 1px solid rgba(255, 255, 255, 0.1);"
"            padding: 30px;"
"            border-radius: 15px;"
"            box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);"
"            width: 320px;"
"            text-align: center;"
"        }"
"        h2 {"
"            margin-bottom: 20px;"
"            color: #00adb5;"
"        }"
"        .input-group {"
"            margin-bottom: 15px;"
"            text-align: left;"
"        }"
"        label {"
"            display: block;"
"            margin-bottom: 5px;"
"            font-size: 14px;"
"            color: #aaaaaa;"
"        }"
"        input[type=\"text\"], input[type=\"password\"] {"
"            width: 100%;"
"            padding: 10px;"
"            box-sizing: border-box;"
"            border: 1px solid rgba(255, 255, 255, 0.2);"
"            background: rgba(0, 0, 0, 0.2);"
"            color: #ffffff;"
"            border-radius: 5px;"
"            outline: none;"
"            transition: 0.3s;"
"        }"
"        input:focus {"
"            border-color: #00adb5;"
"            box-shadow: 0 0 8px rgba(0, 173, 181, 0.5);"
"        }"
"        button {"
"            width: 100%;"
"            padding: 12px;"
"            background: #00adb5;"
"            border: none;"
"            color: white;"
"            font-size: 16px;"
"            font-weight: bold;"
"            border-radius: 5px;"
"            cursor: pointer;"
"            transition: 0.3s;"
"            margin-top: 10px;"
"        }"
"        button:hover {"
"            background: #00bfa5;"
"            box-shadow: 0 0 15px rgba(0, 191, 165, 0.6);"
"        }"
"        .footer {"
"            margin-top: 20px;"
"            font-size: 11px;"
"            color: #666666;"
"        }"
"    </style>"
"</head>"
"<body>"
"    <div class=\"container\">"
"        <h2>ESP32 WiFi Config</h2>"
"        <form action=\"/config\" method=\"POST\">"
"            <div class=\"input-group\">"
"                <label for=\"ssid\">WiFi SSID</label>"
"                <input type=\"text\" id=\"ssid\" name=\"ssid\" required placeholder=\"Enter SSID\">"
"            </div>"
"            <div class=\"input-group\">"
"                <label for=\"password\">Password</label>"
"                <input type=\"password\" id=\"password\" name=\"password\" placeholder=\"Enter Password\">"
"            </div>"
"            <button type=\"submit\">Save & Connect</button>"
"        </form>"
"        <div class=\"footer\">Antigravity ESP-IDF WiFi Manager</div>"
"    </div>"
"</body>"
"</html>";

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
}

void restart_task(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Restarting ESP32 device...");
    esp_restart();
}

static esp_err_t config_post_handler(httpd_req_t *req)
{
    char buf[256];
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

    char raw_ssid[64] = {0};
    char raw_pass[64] = {0};
    char ssid[64] = {0};
    char password[64] = {0};

    char* ssid_ptr = strstr(buf, "ssid=");
    if (ssid_ptr) {
        ssid_ptr += 5;
        char* end = strchr(ssid_ptr, '&');
        if (end) {
            strncpy(raw_ssid, ssid_ptr, end - ssid_ptr);
        } else {
            strcpy(raw_ssid, ssid_ptr);
        }
    }

    char* pass_ptr = strstr(buf, "password=");
    if (pass_ptr) {
        pass_ptr += 9;
        char* end = strchr(pass_ptr, '&');
        if (end) {
            strncpy(raw_pass, pass_ptr, end - pass_ptr);
        } else {
            strcpy(raw_pass, pass_ptr);
        }
    }

    url_decode(ssid, raw_ssid);
    url_decode(password, raw_pass);

    ESP_LOGI(TAG, "Saving credentials: SSID='%s'", ssid);

    esp_err_t err = save_wifi_credentials(ssid, password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save credentials to NVS!");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save credentials");
        return ESP_FAIL;
    }

    const char* success_html_1 = 
    "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<title>Success</title><style>"
    "body { font-family: sans-serif; background: #121214; color: #fff; text-align: center; padding-top: 50px; }"
    "h2 { color: #00adb5; }"
    "</style></head><body>"
    "<h2>WiFi Credentials Saved!</h2>"
    "<p>ESP32 will restart and connect to <strong>";
    
    httpd_resp_sendstr_chunk(req, success_html_1);
    httpd_resp_sendstr_chunk(req, ssid);
    httpd_resp_sendstr_chunk(req, "</strong> shortly.</p></body></html>");
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

    // Read wifi configuration from NVS
    char ssid[64] = {0};
    char password[64] = {0};
    esp_err_t err = read_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password));

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
    }
}
