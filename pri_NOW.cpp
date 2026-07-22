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
#include "wifi_config_html.h"
#include "Arduino.h"
#include "mqtt_handler.h"
#include "driver/gpio.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "thermal_printer.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

ThermalPrinter g_printer(UART_NUM_2);
char g_unit_name[128] = "HE THONG XEP HANG";

void init_thermal_printer() {
    uart_config_t uart_config;
    memset(&uart_config, 0, sizeof(uart_config_t));
    uart_config.baud_rate = 9600;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity    = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, GPIO_NUM_18, GPIO_NUM_17, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_2, 1024, 2048, 0, NULL, 0);
    ESP_LOGI("PRINTER", "Thermal Printer UART initialized on TX=18, RX=17 (Baud: 9600, TX Buf: 2048)");
}
#include <mutex>

static const char *TAG = "wifi_manager";
static bool g_in_ap_mode = false;

static std::string g_scan_result_json = "[]";
static std::mutex  g_scan_mutex;
static volatile bool g_scan_in_progress = false;
static int wifi_rssi_to_quality(int rssi); // forward declare
static int g_ap_config_counter = 180;      // forward declare (used by wifi_connect_switch_task)

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

esp_err_t save_gpio_config(const char* json_str) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("gpio_cfg", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;
    nvs_set_str(my_handle, "json", json_str);
    nvs_commit(my_handle);
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t read_gpio_config(char* json_str, size_t max_len) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("gpio_cfg", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        json_str[0] = '\0';
        return err;
    }
    if (nvs_get_str(my_handle, "json", json_str, &max_len) != ESP_OK) json_str[0] = '\0';
    nvs_close(my_handle);
    return ESP_OK;
}

// ==================== WIFI EVENT HANDLER ====================
// Event handler for wifi and IP events
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        add_device_log("Connecting to AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_wifi_event_group != NULL) {
            // Boot phase: limited retries
            if (s_retry_num < MAXIMUM_RETRY) {
                esp_wifi_connect();
                s_retry_num++;
                add_device_log("Retry connection to AP (%d/%d)", s_retry_num, MAXIMUM_RETRY);
            } else {
                add_device_log("WiFi connection failed after max retries.");
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
        } else {
            // Runtime phase: reconnect directly without blocking event loop
            static int s_runtime_disconnect_count = 0;
            s_runtime_disconnect_count++;
            add_device_log("WiFi lost during runtime (%d retries). Retrying connection...", s_runtime_disconnect_count);
            if (s_runtime_disconnect_count > 10 && !g_in_ap_mode) {
                add_device_log("WiFi connection lost repeatedly. Re-enabling AP mode for reconfiguration...");
                g_in_ap_mode = true;
                esp_wifi_set_mode(WIFI_MODE_APSTA);
                s_runtime_disconnect_count = 0;
            }
            esp_wifi_connect();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        uint16_t number = 16;
        wifi_ap_record_t ap_records[16];
        esp_wifi_scan_get_ap_records(&number, ap_records);
        add_device_log("WiFi Scan complete. Found %d networks.", number);

        cJSON *root = cJSON_CreateArray();
        for (int i = 0; i < number; i++) {
            if (strlen((char*)ap_records[i].ssid) > 0) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "ssid", (char*)ap_records[i].ssid);
                cJSON_AddNumberToObject(item, "rssi", ap_records[i].rssi);
                cJSON_AddNumberToObject(item, "quality", wifi_rssi_to_quality(ap_records[i].rssi));
                const char* auth_str = "OPEN";
                if (ap_records[i].authmode == WIFI_AUTH_WEP) auth_str = "WEP";
                else if (ap_records[i].authmode == WIFI_AUTH_WPA_PSK) auth_str = "WPA";
                else if (ap_records[i].authmode == WIFI_AUTH_WPA2_PSK) auth_str = "WPA2";
                else if (ap_records[i].authmode == WIFI_AUTH_WPA_WPA2_PSK) auth_str = "WPA/WPA2";
                else if (ap_records[i].authmode == WIFI_AUTH_WPA3_PSK) auth_str = "WPA3";
                cJSON_AddStringToObject(item, "auth", auth_str);
                cJSON_AddItemToArray(root, item);
            }
        }
        char *json_str = cJSON_PrintUnformatted(root);
        {
            std::lock_guard<std::mutex> lock(g_scan_mutex);
            g_scan_result_json = json_str ? json_str : "[]";
        }
        if (json_str) free(json_str);
        cJSON_Delete(root);
        g_scan_in_progress = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        char ip_str[32];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        add_device_log("Successfully got IP: %s", ip_str);
        s_retry_num = 0;
        if (s_wifi_event_group != NULL) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }

        // Auto-start MQTT client when IP is obtained if configured and not yet running
        if (mqtt_client == NULL) {
            char mqtt_server[64] = {0}, mqtt_port[16] = {0}, mqtt_user[64] = {0}, mqtt_pass[64] = {0}, mqtt_topic[256] = {0};
            read_mqtt_config(mqtt_server, sizeof(mqtt_server), mqtt_port, sizeof(mqtt_port), mqtt_user, sizeof(mqtt_user), mqtt_pass, sizeof(mqtt_pass), mqtt_topic, sizeof(mqtt_topic));
            if (strlen(mqtt_server) > 0) {
                int port = 1883;
                if (strlen(mqtt_port) > 0) port = atoi(mqtt_port);
                add_device_log("Network ready. Starting MQTT connection to %s:%d...", mqtt_server, port);
                mqtt_app_start(mqtt_server, port, mqtt_user, mqtt_pass, mqtt_topic);
            }
        }
    }
}

// Helper function to decode URL-encoded string safely
void url_decode(char *dst, const char *src, size_t dst_max) {
    if (dst_max == 0) return;
    char a = 0, b = 0;
    size_t written = 0;
    while (*src && (written < dst_max - 1)) {
        if ((*src == '%') &&
            (src[1] && src[2]) &&
            (isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2]))) {
            a = src[1];
            b = src[2];
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
        written++;
    }
    *dst = '\0';
}

// Helper function to extract and decode a URL parameter safely
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
    
    char* raw = (char*)malloc(len + 1);
    if (!raw) {
        dst[0] = '\0';
        return false;
    }
    strncpy(raw, ptr, len);
    raw[len] = '\0';
    
    url_decode(dst, raw, dst_max);
    free(raw);
    return true;
}

// Helper function to replace placeholders in a string
static std::string replace_placeholder(std::string str, const std::string& placeholder, const std::string& replacement) {
    size_t pos = 0;
    while ((pos = str.find(placeholder, pos)) != std::string::npos) {
        str.replace(pos, placeholder.length(), replacement);
        pos += replacement.length();
    }
    return str;
}

static bool is_authorized(httpd_req_t *req)
{
    if (g_in_ap_mode) {
        return true; // Always allow configuration access in AP mode
    }
    size_t hdr_len = httpd_req_get_hdr_value_len(req, "Cookie");
    if (hdr_len == 0) {
        return false;
    }
    char* cookie_buf = (char*)malloc(hdr_len + 1);
    if (!cookie_buf) {
        return false;
    }
    esp_err_t err = httpd_req_get_hdr_value_str(req, "Cookie", cookie_buf, hdr_len + 1);
    bool authorized = false;
    if (err == ESP_OK) {
        if (strstr(cookie_buf, "passwd=thien1991") != NULL) {
            authorized = true;
        } else {
            ESP_LOGI(TAG, "Cookie found but password not matched: %s", cookie_buf);
        }
    } else {
        if (err != ESP_ERR_NOT_FOUND) {
            ESP_LOGI(TAG, "Cookie header error: %d", err);
        }
    }
    free(cookie_buf);
    return authorized;
}

// Helper: send large data in 4KB chunks for high performance web serving
#define CHUNK_SEND_SIZE 4096
static esp_err_t send_large_chunk(httpd_req_t *req, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        size_t to_send = len - sent;
        if (to_send > CHUNK_SEND_SIZE) to_send = CHUNK_SEND_SIZE;
        if (httpd_resp_send_chunk(req, data + sent, to_send) != ESP_OK) {
            return ESP_FAIL;
        }
        sent += to_send;
    }
    return ESP_OK;
}

// Helper function to send HTML template using chunked response to avoid large heap allocations
static esp_err_t send_html_template_chunked(httpd_req_t *req, const char* template_str, 
                                const char* ssid, const char* password,
                                const char* mqtt_server, const char* mqtt_port,
                                const char* mqtt_user, const char* mqtt_pass,
                                const char* mqtt_topic, const char* ws_url,
                                const char* dev_id, const char* dev_key) {
    std::string html = template_str;
    html = replace_placeholder(html, "{{SSID}}", strlen(ssid) > 0 ? ssid : "");
    html = replace_placeholder(html, "{{PASSWORD}}", strlen(password) > 0 ? password : "");
    html = replace_placeholder(html, "{{MQTT_SERVER}}", strlen(mqtt_server) > 0 ? mqtt_server : "qms1.camdvr.org");
    html = replace_placeholder(html, "{{MQTT_PORT}}", strlen(mqtt_port) > 0 ? mqtt_port : "1993");
    html = replace_placeholder(html, "{{MQTT_USER}}", strlen(mqtt_user) > 0 ? mqtt_user : "thom");
    html = replace_placeholder(html, "{{MQTT_PASS}}", strlen(mqtt_pass) > 0 ? mqtt_pass : "301258");
    html = replace_placeholder(html, "{{MQTT_TOPIC}}", strlen(mqtt_topic) > 0 ? mqtt_topic : "qms/display");
    html = replace_placeholder(html, "{{WS_URL}}", strlen(ws_url) > 0 ? ws_url : "");
    html = replace_placeholder(html, "{{DEV_ID}}", strlen(dev_id) > 0 ? dev_id : "");
    html = replace_placeholder(html, "{{DEV_KEY}}", strlen(dev_key) > 0 ? dev_key : "");

    if (send_large_chunk(req, html.c_str(), html.length()) != ESP_OK) {
        return ESP_FAIL;
    }
    return httpd_resp_send_chunk(req, NULL, 0);
}

// Helper function to send Log template using chunked response
static esp_err_t send_log_template_chunked(httpd_req_t *req, const char* template_str, const char* dev_id, const char* dev_key) {
    std::string html = template_str;
    html = replace_placeholder(html, "{{DEV_ID}}", strlen(dev_id) > 0 ? dev_id : "");
    html = replace_placeholder(html, "{{DEV_KEY}}", strlen(dev_key) > 0 ? dev_key : "");

    if (send_large_chunk(req, html.c_str(), html.length()) != ESP_OK) {
        return ESP_FAIL;
    }
    return httpd_resp_send_chunk(req, NULL, 0);
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    if (!is_authorized(req)) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/login");
        httpd_resp_sendstr(req, "Redirecting...");
        return ESP_OK;
    }
    char ssid[64] = {0};
    char password[64] = {0};
    char mqtt_server[64] = {0};
    char mqtt_port[16] = {0};
    char mqtt_user[64] = {0};
    char mqtt_pass[64] = {0};
    char mqtt_topic[256] = {0};
    char ws_url[128] = {0};
    char dev_id[128] = {0};
    char dev_key[128] = {0};

    // Read current config
    read_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password));
    read_mqtt_config(mqtt_server, sizeof(mqtt_server), mqtt_port, sizeof(mqtt_port), mqtt_user, sizeof(mqtt_user), mqtt_pass, sizeof(mqtt_pass), mqtt_topic, sizeof(mqtt_topic));
    read_ws_config(ws_url, sizeof(ws_url));
    read_dev_credentials(dev_id, sizeof(dev_id), dev_key, sizeof(dev_key));

    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    return send_html_template_chunked(req, html_page, ssid, password, mqtt_server, mqtt_port, mqtt_user, mqtt_pass, mqtt_topic, ws_url, dev_id, dev_key);
}

static void wifi_connect_switch_task(void *pv) {
    char *ssid = (char*)pv;
    esp_netif_ip_info_t ip_info;
    esp_netif_t* netif_sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    bool got_ip = false;
    char got_ip_str[32] = {0};

    for (int i = 0; i < 20; i++) { // tối đa 10 giây
        vTaskDelay(pdMS_TO_TICKS(500));
        if (netif_sta && esp_netif_get_ip_info(netif_sta, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
            snprintf(got_ip_str, sizeof(got_ip_str), IPSTR, IP2STR(&ip_info.ip));
            got_ip = true;
            break;
        }
    }

    if (got_ip) {
        add_device_log("Live connection successful! IP: %s. Switching off AP (no reboot)...", got_ip_str);
        if (g_in_ap_mode) {
            g_in_ap_mode = false;          // ap_timeout_task tự thoát ở lần check kế tiếp
            g_ap_config_counter = 0;
            esp_wifi_set_mode(WIFI_MODE_STA); // tắt AP, giữ nguyên phiên STA đang kết nối
        }
        // Khởi động MQTT nếu chưa chạy và đã có cấu hình broker
        if (mqtt_client == NULL) {
            char mqtt_server[64]={0}, mqtt_port[16]={0}, mqtt_user[64]={0}, mqtt_pass[64]={0}, mqtt_topic[256]={0};
            read_mqtt_config(mqtt_server, sizeof(mqtt_server), mqtt_port, sizeof(mqtt_port),
                              mqtt_user, sizeof(mqtt_user), mqtt_pass, sizeof(mqtt_pass),
                              mqtt_topic, sizeof(mqtt_topic));
            if (strlen(mqtt_server) > 0) {
                int port = strlen(mqtt_port) > 0 ? atoi(mqtt_port) : 1883;
                mqtt_app_start(mqtt_server, port, mqtt_user, mqtt_pass, mqtt_topic);
            }
        }
    } else {
        add_device_log("Failed to connect to WiFi SSID: %s. Keeping AP active for retry.", ssid);
    }

    free(ssid);
    vTaskDelete(NULL);
}

void restart_task(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Restarting ESP32 device...");
    esp_restart();
}

static esp_err_t config_post_handler(httpd_req_t *req)
{
    if (!is_authorized(req)) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
        return ESP_OK;
    }
    int remaining = req->content_len;
    if (remaining <= 0 || remaining > 4096) {
        ESP_LOGE(TAG, "POST content length invalid or too large: %d", remaining);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content length");
        return ESP_FAIL;
    }
    char *buf = (char*)malloc(remaining + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    int cur_len = 0;
    int ret;
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf + cur_len, remaining)) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            free(buf);
            return ESP_FAIL;
        }
        cur_len += ret;
        remaining -= ret;
    }
    buf[cur_len] = '\0';

    char config_section[32] = {0};
    parse_url_param(buf, "config_section", config_section, sizeof(config_section));

    ESP_LOGI(TAG, "Saving configurations for section: %s", config_section);
    esp_err_t err = ESP_OK;

    if (strcmp(config_section, "NETWORK") == 0) {
        char ssid[64] = {0};
        char password[64] = {0};
        parse_url_param(buf, "ssid", ssid, sizeof(ssid));
        parse_url_param(buf, "password", password, sizeof(password));
        err = save_wifi_credentials(ssid, password);
        add_device_log("Network configuration saved. Connecting in background to SSID: %s...", ssid);
        free(buf);

        if (err != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save configuration");
            return ESP_FAIL;
        }

        // Kết nối chạy nền — không block httpd, không cần reboot
        wifi_config_t wifi_config = {};
        strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_connect();

        char response_html[768];
        snprintf(response_html, sizeof(response_html),
            "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
            "<title>WiFi Saved</title></head><body style=\"font-family:sans-serif;background:#0b0f14;color:#fff;text-align:center;padding-top:50px;\">"
            "<h2 style=\"color:#5ec98f;\">Wi-Fi Saved!</h2>"
            "<p>Connecting to <b>%s</b>...</p>"
            "<p style=\"color:#8b949e;font-size:13px;\">AP will auto-disable on success. If no IP in ~10s, reconnect to ESP32_WiFi_Config.</p>"
            "</body></html>", ssid);

        httpd_resp_set_hdr(req, "Connection", "close");
        httpd_resp_sendstr(req, response_html);

        char *ssid_copy = strdup(ssid);
        xTaskCreate(wifi_connect_switch_task, "wifi_switch_task", 3072, ssid_copy, 5, NULL);
        return ESP_OK;
    } 
    else if (strcmp(config_section, "MQTT") == 0) {
        char mqtt_server[64] = {0};
        char mqtt_port[16] = {0};
        char mqtt_user[64] = {0};
        char mqtt_pass[64] = {0};
        char mqtt_topic[256] = {0};
        parse_url_param(buf, "mqtt_server", mqtt_server, sizeof(mqtt_server));
        parse_url_param(buf, "mqtt_port", mqtt_port, sizeof(mqtt_port));
        parse_url_param(buf, "mqtt_user", mqtt_user, sizeof(mqtt_user));
        parse_url_param(buf, "mqtt_pass", mqtt_pass, sizeof(mqtt_pass));
        err = save_mqtt_config(mqtt_server, mqtt_port, mqtt_user, mqtt_pass, mqtt_topic);
        add_device_log("MQTT configuration saved. Re-initializing MQTT connection...");

        // Stop existing MQTT client if running
        if (mqtt_client != NULL) {
            esp_mqtt_client_stop(mqtt_client);
            esp_mqtt_client_destroy(mqtt_client);
            mqtt_client = NULL;
        }

        // Start new MQTT connection immediately
        if (strlen(mqtt_server) > 0) {
            int port = 1883;
            if (strlen(mqtt_port) > 0) port = atoi(mqtt_port);
            mqtt_app_start(mqtt_server, port, mqtt_user, mqtt_pass, mqtt_topic);
        }
    }
    else if (strcmp(config_section, "WS") == 0) {
        char ws_url[128] = {0};
        parse_url_param(buf, "ws_url", ws_url, sizeof(ws_url));
        err = save_ws_config(ws_url);
        add_device_log("WebSocket configuration saved.");
    }
    else if (strcmp(config_section, "DEV") == 0) {
        char dev_id[128] = {0};
        char dev_key[128] = {0};
        parse_url_param(buf, "dev_id", dev_id, sizeof(dev_id));
        parse_url_param(buf, "dev_key", dev_key, sizeof(dev_key));
        err = save_dev_credentials(dev_id, dev_key);
        add_device_log("Device credentials saved.");
    } else {
        ESP_LOGE(TAG, "Unknown config_section: %s", config_section);
    }
    
    free(buf);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save configuration to NVS!");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save configuration");
        return ESP_FAIL;
    }

    const char* success_html_1 = 
    "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<title>Success</title><style>"
    "body { font-family: sans-serif; background: #11161d; color: #fff; text-align: center; padding-top: 50px; }"
    "h2 { color: #5ec98f; }"
    ".btn { display: inline-block; margin-top: 20px; padding: 10px 20px; background: #5ec98f; color: #0b0f14; text-decoration: none; font-weight: bold; border-radius: 4px; }"
    "</style>"
    "<script>"
    "setTimeout(function() { window.location.href = '/log'; }, 4000);"
    "</script>"
    "</head><body>"
    "<h2>Configuration Saved Successfully!</h2>"
    "<p>ESP32 is rebooting. You will be redirected to the System Logs page in 4 seconds...</p>"
    "<a href='/log' class=\"btn\">Go to Logs Now</a></body></html>";
    
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_sendstr_chunk(req, success_html_1);
    httpd_resp_sendstr_chunk(req, NULL);

    xTaskCreate(restart_task, "restart_task", 2048, NULL, 5, NULL);

    return ESP_OK;
}

static esp_err_t log_get_handler(httpd_req_t *req)
{
    if (!is_authorized(req)) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/login");
        httpd_resp_sendstr(req, "Redirecting...");
        return ESP_OK;
    }
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    return send_log_template_chunked(req, log_page, g_dev_id, g_dev_key);
}


static esp_err_t gpio_get_handler(httpd_req_t *req)
{
    if (!is_authorized(req)) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/login");
        httpd_resp_sendstr(req, "Redirecting...");
        return ESP_OK;
    }
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    return send_log_template_chunked(req, gpio_page, g_dev_id, g_dev_key);
}

static esp_err_t log_data_get_handler(httpd_req_t *req)
{
    if (!is_authorized(req)) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
        return ESP_OK;
    }
    std::string all_logs = "";
    {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        for (const auto& log_line : g_device_logs) {
            all_logs += log_line + "\n";
        }
    }
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(req, "Connection", "close");
    return httpd_resp_send(req, all_logs.c_str(), all_logs.length());
}

static esp_err_t publish_post_handler(httpd_req_t *req)
{
    if (!is_authorized(req)) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
        return ESP_OK;
    }
    int remaining = req->content_len;
    if (remaining <= 0 || remaining > 2048) {
        ESP_LOGE(TAG, "POST content length invalid or too large: %d", remaining);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content length");
        return ESP_FAIL;
    }
    char *buf = (char*)malloc(remaining + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    int cur_len = 0;
    int ret;
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf + cur_len, remaining)) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            free(buf);
            return ESP_FAIL;
        }
        cur_len += ret;
        remaining -= ret;
    }
    buf[cur_len] = '\0';

    char topic[256] = {0};
    char payload[1024] = {0};

    parse_url_param(buf, "topic", topic, sizeof(topic));
    parse_url_param(buf, "payload", payload, sizeof(payload));
    free(buf);

    if (strlen(topic) > 0 && strlen(payload) > 0) {
        bool local_processed = false;
        if (strcmp(topic, "qms/display") == 0) {
            cJSON *root = cJSON_Parse(payload);
            if (root) {
                cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
                if (cmd && cJSON_IsString(cmd)) {
                    if (strcmp(cmd->valuestring, "display_ticket") == 0) {
                        cJSON *data = cJSON_GetObjectItem(root, "data");
                        if (data) {
                            const char *ticket = cJSON_GetStringValue(cJSON_GetObjectItem(data, "ticket"));
                            const char *color = cJSON_GetStringValue(cJSON_GetObjectItem(data, "color"));
                            local_processed = true;
                        }
                    } else if (strcmp(cmd->valuestring, "clear_display") == 0) {
                        local_processed = true;
                    } else if (strcmp(cmd->valuestring, "print_ticket") == 0) {
                        cJSON *data = cJSON_GetObjectItem(root, "data");
                        if (data) {
                            const char *ticket = cJSON_GetStringValue(cJSON_GetObjectItem(data, "ticket"));
                            const char *service = cJSON_GetStringValue(cJSON_GetObjectItem(data, "service"));
                            const char *cust_name = cJSON_GetStringValue(cJSON_GetObjectItem(data, "cust_name"));
                            add_device_log(">>> PRINTING TICKET VIA WEB: Ticket=%s, Service=%s", ticket ? ticket : "N/A", service ? service : "N/A");
                            print_qms_ticket(g_printer, g_unit_name, service, ticket, cust_name);
                            local_processed = true;
                        }
                    }
                }
                cJSON_Delete(root);
            }
            if (!local_processed) {
                local_processed = true;
            }
        }

        if (mqtt_client) {
            int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
            add_device_log("Web Publish: Topic='%s', MsgID=%d, Payload='%s'", topic, msg_id, payload);
            httpd_resp_set_hdr(req, "Connection", "close");
            httpd_resp_sendstr(req, "SUCCESS");
            return ESP_OK;
        } else if (local_processed) {
            add_device_log("Web Publish (Local Only): Topic='%s', Payload='%s'", topic, payload);
            httpd_resp_set_hdr(req, "Connection", "close");
            httpd_resp_sendstr(req, "SUCCESS (LOCAL ONLY)");
            return ESP_OK;
        } else {
            add_device_log("Web Publish Failed: MQTT client not connected.");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "MQTT client not initialized or connected");
            return ESP_FAIL;
        }
    }

    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing topic or payload");
    return ESP_FAIL;
}

static esp_err_t api_test_print_get_handler(httpd_req_t *req)
{
    if (!is_authorized(req)) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
        return ESP_OK;
    }
    add_device_log(">>> TEST PRINT INITIATED FROM WEB API <<<");
    print_qms_ticket(g_printer, g_unit_name, "DICH VU TEST", "A001", "KHACH HANG TEST");
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_sendstr(req, "Test print sent to UART2 printer!");
    return ESP_OK;
}

static esp_err_t login_get_handler(httpd_req_t *req)
{
    extern const char* login_page;
    
    char query[32] = {0};
    bool has_error = false;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (strstr(query, "error=1") != NULL) {
            has_error = true;
        }
    }
    
    std::string html = login_page;
    if (has_error) {
        html = replace_placeholder(html, "{{ERROR_STYLE}}", "display: block;");
    } else {
        html = replace_placeholder(html, "{{ERROR_STYLE}}", "display: none;");
    }
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    return httpd_resp_send(req, html.c_str(), html.length());
}

static esp_err_t login_post_handler(httpd_req_t *req)
{
    int remaining = req->content_len;
    ESP_LOGI(TAG, "Login POST received. Content-Length: %d", remaining);
    if (remaining <= 0 || remaining > 512) {
        ESP_LOGE(TAG, "POST content length invalid or too large: %d", remaining);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content length");
        return ESP_FAIL;
    }
    char *buf = (char*)malloc(remaining + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    int cur_len = 0;
    int ret;
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf + cur_len, remaining)) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            free(buf);
            return ESP_FAIL;
        }
        cur_len += ret;
        remaining -= ret;
    }
    buf[cur_len] = '\0';

    char password[64] = {0};
    parse_url_param(buf, "password", password, sizeof(password));
    free(buf);

    ESP_LOGI(TAG, "Login attempt. Parsed password: '%s'", password);

    if (strcmp(password, "thien1991") == 0) {
        ESP_LOGI(TAG, "Login successful! Setting cookie and redirecting to /");
        httpd_resp_set_hdr(req, "Connection", "close");
        httpd_resp_set_hdr(req, "Set-Cookie", "passwd=thien1991; Path=/");
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/");
        httpd_resp_sendstr(req, "Redirecting...");
        return ESP_OK;
    } else {
        ESP_LOGI(TAG, "Login failed! Redirecting to /login?error=1");
        httpd_resp_set_hdr(req, "Connection", "close");
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/login?error=1");
        httpd_resp_sendstr(req, "Redirecting...");
        return ESP_OK;
    }
}


// ==================== GPIO API HANDLERS ====================
static esp_err_t api_services_get_handler(httpd_req_t *req)
{
    if (!is_authorized(req)) return ESP_OK;
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Connection", "close");
    
    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < g_service_count; i++) {
        cJSON *srv = cJSON_CreateObject();
        cJSON_AddNumberToObject(srv, "id", g_services[i].id);
        cJSON_AddStringToObject(srv, "name", g_services[i].name);
        cJSON_AddItemToArray(root, srv);
    }
    char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_send(req, json_str, strlen(json_str));
    free(json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t api_gpio_config_get_handler(httpd_req_t *req)
{
    if (!is_authorized(req)) return ESP_OK;
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Connection", "close");
    
    char saved_json[1024] = {0};
    if (read_gpio_config(saved_json, sizeof(saved_json)) != ESP_OK || strlen(saved_json) == 0) {
        strcpy(saved_json, "{}");
    }
    httpd_resp_send(req, saved_json, strlen(saved_json));
    return ESP_OK;
}

static esp_err_t api_gpio_config_post_handler(httpd_req_t *req)
{
    if (!is_authorized(req)) return ESP_OK;
    
    int remaining = req->content_len;
    if (remaining <= 0 || remaining > 1024) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content length");
        return ESP_FAIL;
    }
    char *buf = (char*)malloc(remaining + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    int cur_len = 0;
    int ret;
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf + cur_len, remaining)) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            free(buf);
            return ESP_FAIL;
        }
        cur_len += ret;
        remaining -= ret;
    }
    buf[cur_len] = '\0';

    esp_err_t err = save_gpio_config(buf);
    free(buf);
    
    if (err == ESP_OK) {
        httpd_resp_sendstr(req, "OK");
        add_device_log("GPIO Config saved. Restarting to apply...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save");
    }
    return ESP_OK;
}

static int wifi_rssi_to_quality(int rssi) {
    if (rssi <= -100) return 0;
    if (rssi >= -50) return 100;
    return 2 * (rssi + 100);
}

// DNS Server Task for Tasmota-style Automatic Captive Portal Pop-up
static void dns_server_task(void *pvParameters) {
    uint8_t rx_buffer[128];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        add_device_log("DNS Server: Socket creation failed");
        vTaskDelete(NULL);
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53); // Port 53 DNS
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        add_device_log("DNS Server: Port 53 bind failed");
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    add_device_log("DNS Server (Captive Portal) running on port 53");

    while (g_in_ap_mode) {
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (len > 12) {
            uint8_t reply[256];
            memcpy(reply, rx_buffer, len);
            reply[2] |= 0x80; // QR = 1
            reply[3] |= 0x80; // RA = 1
            reply[6] = 0x00; reply[7] = 0x01; // Answer Count = 1

            int idx = len;
            reply[idx++] = 0xc0; reply[idx++] = 0x0c;
            reply[idx++] = 0x00; reply[idx++] = 0x01; // Type A
            reply[idx++] = 0x00; reply[idx++] = 0x01; // Class IN
            reply[idx++] = 0x00; reply[idx++] = 0x00; reply[idx++] = 0x00; reply[idx++] = 0x3c; // TTL 60s
            reply[idx++] = 0x00; reply[idx++] = 0x04; // RDLENGTH 4
            reply[idx++] = 192;  reply[idx++] = 168; reply[idx++] = 4;    reply[idx++] = 1;   // 192.168.4.1

            sendto(sock, reply, idx, 0, (struct sockaddr *)&client_addr, client_addr_len);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    close(sock);
    vTaskDelete(NULL);
}

// Captive Portal 302 Redirect Handler
static esp_err_t captive_portal_redirect_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_sendstr(req, "Redirecting to WiFi Setup...");
    return ESP_OK;
}

// WiFi Scan API Endpoint (/api/scan-wifi)
static esp_err_t api_scan_wifi_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Connection", "close");

    char query[16];
    bool start_scan = (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) &&
                       (strstr(query, "start=1") != NULL);

    if (start_scan && !g_scan_in_progress) {
        wifi_scan_config_t scan_config = {};
        scan_config.show_hidden = false;
        scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
        if (esp_wifi_scan_start(&scan_config, false) == ESP_OK) { // false = non-blocking
            g_scan_in_progress = true;
        }
    }

    std::string result;
    {
        std::lock_guard<std::mutex> lock(g_scan_mutex);
        result = g_scan_result_json;
    }
    std::string payload = "{\"scanning\":" + std::string(g_scan_in_progress ? "true" : "false") +
                           ",\"networks\":" + result + "}";
    httpd_resp_send(req, payload.c_str(), payload.length());
    return ESP_OK;
}


inline void reset_wifi_config_counter(void) {
    g_ap_config_counter = 180;
}

// AP Mode Timeout Monitor (Tasmota Style: 180 seconds auto-restart)
static void ap_timeout_task(void *pvParameters) {
    g_ap_config_counter = 180;
    add_device_log("AP Mode Timeout Monitor active (%d seconds timeout)", g_ap_config_counter);
    while (g_in_ap_mode && g_ap_config_counter > 0) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        g_ap_config_counter--;
    }
    if (g_in_ap_mode && g_ap_config_counter <= 0) {
        add_device_log("AP Timeout reached (180s inactive). Restarting device to retry STA connection...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }
    vTaskDelete(NULL);
}

static int s_local_ticket_counter[100] = {0};

void print_ticket_by_service_id(int service_id, const char* customer_name = "Nút Bấm Trực Tiếp") {
    const char* service_name = "Dịch Vụ";
    for (int j = 0; j < g_service_count; j++) {
        if (g_services[j].id == service_id) {
            service_name = g_services[j].name;
            break;
        }
    }
    
    int sid_idx = service_id % 100;
    s_local_ticket_counter[sid_idx]++;
    if (s_local_ticket_counter[sid_idx] > 999) s_local_ticket_counter[sid_idx] = 1;
    
    char ticket_num[16];
    snprintf(ticket_num, sizeof(ticket_num), "%03d", s_local_ticket_counter[sid_idx]);
    
    add_device_log(">>> IN PHIẾU NÚT BẤM (Service ID: %d): Số %s - %s", service_id, ticket_num, service_name);
    print_qms_ticket(g_printer, g_unit_name, service_name, ticket_num, customer_name);
}

// ==================== BUTTON TASK ====================
static void button_task(void *pvParameters) {
    // Parse saved JSON and configure pins
    char saved_json[1024] = {0};
    read_gpio_config(saved_json, sizeof(saved_json));
    
    if (strlen(saved_json) > 0) {
        cJSON *root = cJSON_Parse(saved_json);
        if (root) {
            cJSON *board = cJSON_GetObjectItem(root, "board");
            if (board && cJSON_IsString(board)) {
                strncpy(g_board_type, board->valuestring, sizeof(g_board_type)-1);
            }
            
            cJSON *mappings = cJSON_GetObjectItem(root, "mappings");
            if (mappings && cJSON_IsArray(mappings)) {
                int count = cJSON_GetArraySize(mappings);
                g_pin_mapping_count = 0;
                for (int i = 0; i < count && i < MAX_PIN_MAPPINGS; i++) {
                    cJSON *item = cJSON_GetArrayItem(mappings, i);
                    cJSON *sid = cJSON_GetObjectItem(item, "service_id");
                    cJSON *pin = cJSON_GetObjectItem(item, "pin");
                    if (sid && pin) {
                        g_pin_mappings[g_pin_mapping_count].service_id = sid->valueint;
                        g_pin_mappings[g_pin_mapping_count].pin = pin->valueint;
                        
                        // Setup GPIO
                        gpio_reset_pin((gpio_num_t)pin->valueint);
                        gpio_set_direction((gpio_num_t)pin->valueint, GPIO_MODE_INPUT);
                        gpio_set_pull_mode((gpio_num_t)pin->valueint, GPIO_PULLUP_ONLY);
                        
                        g_pin_mapping_count++;
                    }
                }
                add_device_log("Configured %d GPIO pins for buttons", g_pin_mapping_count);
            }
            cJSON_Delete(root);
        }
    }

    // Default GPIO Fallback if no NVS config exists
    if (g_pin_mapping_count == 0) {
        int default_pins[] = {4, 5, 6, 7};
        for (int i = 0; i < 4; i++) {
            g_pin_mappings[i].service_id = i + 1;
            g_pin_mappings[i].pin = default_pins[i];
            gpio_reset_pin((gpio_num_t)default_pins[i]);
            gpio_set_direction((gpio_num_t)default_pins[i], GPIO_MODE_INPUT);
            gpio_set_pull_mode((gpio_num_t)default_pins[i], GPIO_PULLUP_ONLY);
        }
        g_pin_mapping_count = 4;
        add_device_log("Initialized default 4 button pins: GPIO 4, 5, 6, 7");
    }

    // State tracking for debounce
    int last_state[MAX_PIN_MAPPINGS];
    for (int i=0; i<MAX_PIN_MAPPINGS; i++) last_state[i] = 1; // Pullup default High
    
    while (1) {
        for (int i = 0; i < g_pin_mapping_count; i++) {
            int current_state = gpio_get_level((gpio_num_t)g_pin_mappings[i].pin);
            if (current_state == 0 && last_state[i] == 1) {
                // Button pressed (falling edge)
                add_device_log("Nút bấm GPIO %d được nhấn (Service ID: %d)", g_pin_mappings[i].pin, g_pin_mappings[i].service_id);
                send_ticket_request_by_service_id(g_pin_mappings[i].service_id);
                print_ticket_by_service_id(g_pin_mappings[i].service_id, "Nút Bấm Trực Tiếp");
                vTaskDelay(pdMS_TO_TICKS(500)); // Debounce and block hold
            }
            last_state[i] = current_state;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static httpd_handle_t s_webserver_handle = NULL;


static void ota_task(void *pvParameter)
{
    add_device_log("Starting OTA update from URL: %s", g_ota_url);
    esp_http_client_config_t client_config = {};
    client_config.url = g_ota_url;
    client_config.crt_bundle_attach = esp_crt_bundle_attach;
    client_config.keep_alive_enable = true;
    
    esp_https_ota_config_t ota_config = {};
    ota_config.http_config = &client_config;
    
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        add_device_log("OTA Update successful! Restarting...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    } else {
        add_device_log("OTA Update failed! Error: %s", esp_err_to_name(ret));
    }
    vTaskDelete(NULL);
}

static esp_err_t api_ota_start_post_handler(httpd_req_t *req)
{
    if (strlen(g_ota_url) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No OTA URL available");
        return ESP_FAIL;
    }
    httpd_resp_sendstr(req, "OTA Started");
    xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
    return ESP_OK;
}

static esp_err_t api_ota_check_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    char resp[512];
    snprintf(resp, sizeof(resp), "{\"url\":\"%s\"}", g_ota_url);
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}


static esp_err_t ota_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_send(req, ota_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 32;
    config.max_open_sockets = 7; // Support up to 7 concurrent browser connections
    config.stack_size = 16384;
    config.lru_purge_enable = true;
    config.send_wait_timeout = 5; // 5 seconds timeout to release sockets fast
    config.recv_wait_timeout = 5; // 5 seconds receive timeout

    ESP_LOGI(TAG, "Starting web server on port: %d", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        s_webserver_handle = server;
        httpd_uri_t root = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t login_get = {
            .uri       = "/login",
            .method    = HTTP_GET,
            .handler   = login_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &login_get);

        httpd_uri_t login_post = {
            .uri       = "/login",
            .method    = HTTP_POST,
            .handler   = login_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &login_post);

        httpd_uri_t config_post = {
            .uri       = "/config",
            .method    = HTTP_POST,
            .handler   = config_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &config_post);

        httpd_uri_t log_get = {
            .uri       = "/log",
            .method    = HTTP_GET,
            .handler   = log_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &log_get);



        httpd_uri_t log_data_get = {
            .uri       = "/log_data",
            .method    = HTTP_GET,
            .handler   = log_data_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &log_data_get);

        httpd_uri_t gpio_get = {
            .uri       = "/gpio",
            .method    = HTTP_GET,
            .handler   = gpio_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &gpio_get);

        httpd_uri_t publish_post = {
            .uri       = "/publish",
            .method    = HTTP_POST,
            .handler   = publish_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &publish_post);


        httpd_uri_t api_services_get = {
            .uri       = "/api/services",
            .method    = HTTP_GET,
            .handler   = api_services_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_services_get);

        httpd_uri_t api_gpio_config_get = {
            .uri       = "/api/gpio_config",
            .method    = HTTP_GET,
            .handler   = api_gpio_config_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_gpio_config_get);

        httpd_uri_t api_gpio_config_post = {
            .uri       = "/api/gpio_config",
            .method    = HTTP_POST,
            .handler   = api_gpio_config_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_gpio_config_post);


        httpd_uri_t api_ota_start = {
            .uri       = "/api/ota_start",
            .method    = HTTP_POST,
            .handler   = api_ota_start_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_ota_start);

        httpd_uri_t api_ota_check = {
            .uri       = "/api/ota_check",
            .method    = HTTP_GET,
            .handler   = api_ota_check_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_ota_check);

        httpd_uri_t api_scan_wifi = {
            .uri       = "/api/scan-wifi",
            .method    = HTTP_GET,
            .handler   = api_scan_wifi_get_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t api_test_print = {
            .uri       = "/api/test_print",
            .method    = HTTP_GET,
            .handler   = api_test_print_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_test_print);

        // Tasmota-style Captive Portal HTTP Redirect Endpoints for iOS/Android/Windows
        const char* captive_uris[] = {
            "/generate_204", "/gen_204", "/connecttest.txt", "/redirect",
            "/hotspot-detect.html", "/canonical.html", "/library/test/success.html", "/nconnect.txt"
        };
        for (size_t i = 0; i < sizeof(captive_uris) / sizeof(captive_uris[0]); i++) {
            httpd_uri_t redirect_uri = {
                .uri       = captive_uris[i],
                .method    = HTTP_GET,
                .handler   = captive_portal_redirect_handler,
                .user_ctx  = NULL
            };
            httpd_register_uri_handler(server, &redirect_uri);
        }


        httpd_uri_t ota_get = {
            .uri       = "/ota",
            .method    = HTTP_GET,
            .handler   = ota_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &ota_get);

        return server;
    }

    ESP_LOGE(TAG, "Error starting web server!");
    return NULL;
}


static void terminal_task(void *pvParameters) {
    while (1) {
        int c = fgetc(stdin);
        if (c != EOF) {
            if (c >= '1' && c <= '9') {
                int index = c - '1';
                if (index < g_service_count) {
                    add_device_log("Terminal: Selected service index %d (%s)", index, g_services[index].name);
                    send_ticket_request_by_service_id(g_services[index].id);
                } else {
                    add_device_log("Terminal: Invalid service index %d", index + 1);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

extern "C" void app_main(void)
{
    // Initialize Arduino Core
    initArduino();
    
    // Khởi tạo UART máy in nhiệt
    init_thermal_printer();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS Storage Initialized");

    // Initialize LED Display
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
    char mqtt_topic[256] = {0};
    char ws_url[128] = {0};
    char dev_id[128] = {0};
    char dev_key[128] = {0};
    read_mqtt_config(mqtt_server, sizeof(mqtt_server), mqtt_port, sizeof(mqtt_port), mqtt_user, sizeof(mqtt_user), mqtt_pass, sizeof(mqtt_pass), mqtt_topic, sizeof(mqtt_topic));
    read_ws_config(ws_url, sizeof(ws_url));
    read_dev_credentials(dev_id, sizeof(dev_id), dev_key, sizeof(dev_key));

    // Copy dev credentials to globals immediately so they are available in AP and Station modes
    strncpy(g_dev_id, dev_id, sizeof(g_dev_id) - 1);
    strncpy(g_dev_key, dev_key, sizeof(g_dev_key) - 1);
    g_dev_id[sizeof(g_dev_id) - 1] = '\0';
    g_dev_key[sizeof(g_dev_key) - 1] = '\0';

    add_device_log("---------------------------------------------");
    add_device_log("Loaded Device Configurations from NVS:");
    add_device_log("  WiFi SSID      : %s", strlen(ssid) > 0 ? ssid : "[Not Set]");
    add_device_log("  MQTT Broker    : %s:%s", strlen(mqtt_server) > 0 ? mqtt_server : "[Not Set]", strlen(mqtt_port) > 0 ? mqtt_port : "[Not Set]");
    add_device_log("  MQTT Username  : %s", strlen(mqtt_user) > 0 ? mqtt_user : "[Not Set]");
    add_device_log("  MQTT Topic     : %s", strlen(mqtt_topic) > 0 ? mqtt_topic : "[Not Set]");
    add_device_log("  WebSocket URL  : %s", strlen(ws_url) > 0 ? ws_url : "[Not Set]");
    add_device_log("  Device ID      : %s", strlen(dev_id) > 0 ? dev_id : "[Not Set]");
    add_device_log("  Device KEY     : %s", strlen(dev_key) > 0 ? dev_key : "[Not Set]");
    add_device_log("---------------------------------------------");



    bool connected = false;
    if (err == ESP_OK && strlen(ssid) > 0) {
        add_device_log("Found stored WiFi credentials: SSID='%s'", ssid);
        
        s_wifi_event_group = xEventGroupCreate();
        s_retry_num = 0;

        wifi_config_t wifi_config = {};
        strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        add_device_log("Waiting for WiFi connection...");
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                pdFALSE,
                pdFALSE,
                portMAX_DELAY);

        if (bits & WIFI_CONNECTED_BIT) {
            add_device_log("Successfully connected to WiFi SSID: %s", ssid);
            connected = true;
        } else if (bits & WIFI_FAIL_BIT) {
            add_device_log("Failed to connect to WiFi SSID: %s after %d retries.", ssid, MAXIMUM_RETRY);
        }

        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    } else {
        add_device_log("No stored WiFi credentials found in NVS.");
    }

    if (!connected) {
        g_in_ap_mode = true;
        add_device_log("Starting Access Point and Captive Portal...");
        
        esp_wifi_stop();

        wifi_config_t wifi_config = {};
        const char* ap_ssid = "ESP32_WiFi_Config";
        strncpy((char*)wifi_config.ap.ssid, ap_ssid, sizeof(wifi_config.ap.ssid));
        wifi_config.ap.ssid_len = strlen(ap_ssid);
        wifi_config.ap.channel = 1;
        wifi_config.ap.max_connection = 4;
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        // Limit TX Power to 15dBm (60 * 0.25dBm) like Tasmota to prevent USB power drop resets
        esp_wifi_set_max_tx_power(60);

        add_device_log("AP Mode started. SSID: %s (TX Power: 15dBm, Channel: 1)", ap_ssid);
        add_device_log("Connect to AP - Captive Portal active at http://192.168.4.1");

        start_webserver();

        // Start Tasmota-style DNS Server & AP Timeout Tasks
        xTaskCreate(dns_server_task, "dns_task", 4096, NULL, 5, NULL);
        xTaskCreate(ap_timeout_task, "ap_timeout_task", 4096, NULL, 5, NULL);
    } else {
        add_device_log("Starting Web Server in Station mode...");
        start_webserver();

        // Start MQTT client if broker is configured
        if (strlen(mqtt_server) > 0) {
            int port = 1883;
            if (strlen(mqtt_port) > 0) {
                port = atoi(mqtt_port);
            }
            mqtt_app_start(mqtt_server, port, mqtt_user, mqtt_pass, mqtt_topic);
        } else {
            add_device_log("MQTT Broker not configured. Skipping MQTT connection.");
        }
    }


    xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
    xTaskCreate(terminal_task, "terminal_task", 4096, NULL, 5, NULL);
}
