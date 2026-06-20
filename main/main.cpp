#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
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
#include "mqtt_handler.h"
#include "driver/uart.h"
#include "thermal_printer.h"
#include "kiosk_config_html.h"
#include "driver/gpio.h"
#include "cJSON.h"

esp_err_t read_dev_credentials(char* dev_id, size_t id_len, char* dev_key, size_t key_len);



static const char *TAG = "wifi_manager";


esp_err_t save_kiosk_config(const char* ptr_tx, const char* ptr_rx, const char* btn_cfg) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("kiosk_config", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;
    nvs_set_str(my_handle, "ptr_tx", ptr_tx);
    nvs_set_str(my_handle, "ptr_rx", ptr_rx);
    nvs_set_str(my_handle, "btn_cfg", btn_cfg);
    nvs_commit(my_handle);
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t read_kiosk_config(char* ptr_tx, size_t t_len, char* ptr_rx, size_t r_len, char* btn_cfg, size_t b_len) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("kiosk_config", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        strncpy(ptr_tx, "17", t_len);
        strncpy(ptr_rx, "16", r_len);
        btn_cfg[0] = '\0';
        return err;
    }
    if (nvs_get_str(my_handle, "ptr_tx", ptr_tx, &t_len) != ESP_OK) strncpy(ptr_tx, "17", t_len);
    if (nvs_get_str(my_handle, "ptr_rx", ptr_rx, &r_len) != ESP_OK) strncpy(ptr_rx, "16", r_len);
    if (nvs_get_str(my_handle, "btn_cfg", btn_cfg, &b_len) != ESP_OK) btn_cfg[0] = '\0';
    nvs_close(my_handle);
    return ESP_OK;
}

ThermalPrinter* printer = NULL;

void init_printer() {
    char ptr_tx[16], ptr_rx[16], btn_cfg[1024];
    read_kiosk_config(ptr_tx, sizeof(ptr_tx), ptr_rx, sizeof(ptr_rx), btn_cfg, sizeof(btn_cfg));
    int tx_pin = atoi(ptr_tx) > 0 ? atoi(ptr_tx) : 17;
    int rx_pin = atoi(ptr_rx) > 0 ? atoi(ptr_rx) : 16;

    uart_config_t uart_config = {};
    uart_config.baud_rate = 9600;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_APB;

    uart_driver_install(UART_NUM_2, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    printer = new ThermalPrinter(UART_NUM_2);
}


struct PrintTicketData {
    char ticket[16];
    char counter[32];
    char service[64];
    char cust_name[64];
};
QueueHandle_t printQueue = NULL;

extern "C" void printMQTTTicket(const char* ticket, const char* counter, const char* service, const char* cust_name) {
    if (!printQueue) return;
    PrintTicketData data;
    strncpy(data.ticket, ticket ? ticket : "", sizeof(data.ticket));
    strncpy(data.counter, counter ? counter : "", sizeof(data.counter));
    strncpy(data.service, service ? service : "", sizeof(data.service));
    strncpy(data.cust_name, cust_name ? cust_name : "", sizeof(data.cust_name));
    xQueueSend(printQueue, &data, portMAX_DELAY);
}

void printer_task(void *pvParameters) {
    PrintTicketData data;
    while(1) {
        if(xQueueReceive(printQueue, &data, portMAX_DELAY)) {
            if (!printer) continue;
            
            printer->useHeaderStyle();
            printer->setAlignment(ThermalPrinter::CENTER);
            printer->setSize(2);
            printer->println("SUNSHINE HOSPITAL");
            printer->setSize(1);
            printer->println("156 Nhat Tao P8 Q10 TP HCM");
            printer->setSize(0);
            
            printer->useBodyStyle();
            printer->setAlignment(ThermalPrinter::CENTER);
            printer->println("=================================");
            
            printer->setBold(true);
            if (strlen(data.service) > 0) {
                printer->println(data.service);
            }
            printer->setBold(false);
            
            if (strlen(data.ticket) > 0) {
                printer->setSize(5);
                printer->println(data.ticket);
                printer->setSize(0);
            }
            
            if (strlen(data.cust_name) > 0) {
                printer->setBold(true);
                printer->print("Name: ");
                printer->println(data.cust_name);
                printer->setBold(false);
            }

            if (strlen(data.counter) > 0) {
                printer->print("Counter: ");
                printer->println(data.counter);
            }
            
            printer->println("=================================");
            
            printer->useHeaderStyle();
            printer->setAlignment(ThermalPrinter::CENTER);
            printer->setSize(0);
            printer->println("Please wait for your number");
            printer->println("to be called");
            
            printer->println("");
            printer->println("");
            printer->cut();
            
            vTaskDelay(pdMS_TO_TICKS(500)); // Delay between prints
        }
    }
}




struct BtnMapping {
    int pin;
    char svc[64];
};

#define MAX_BUTTONS 10
BtnMapping g_buttons[MAX_BUTTONS];
int g_button_count = 0;

void button_task(void *pvParameters) {
    uint32_t last_press[MAX_BUTTONS] = {0};
    while(1) {
        for (int i=0; i<g_button_count; i++) {
            if (gpio_get_level((gpio_num_t)g_buttons[i].pin) == 0) { // Pressed (Active Low)
                uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
                if (now - last_press[i] > 1000) { // 1 second debounce
                    last_press[i] = now;
                    ESP_LOGI(TAG, "Button pressed for service: %s", g_buttons[i].svc);
                    
                    // Construct JSON to send via MQTT
                    cJSON *root = cJSON_CreateObject();
                    cJSON_AddStringToObject(root, "cmd", "request_ticket");
                    cJSON_AddStringToObject(root, "service", g_buttons[i].svc);
                    char *json_str = cJSON_PrintUnformatted(root);
                    
                    char dev_id[128] = {0}, dev_key[128] = {0};
                    read_dev_credentials(dev_id, sizeof(dev_id), dev_key, sizeof(dev_key));
                    
                    char topic[256];
                    snprintf(topic, sizeof(topic), "qms/kiosk/%s/request", dev_id);
                    
                    
                    mqtt_publish_message(topic, json_str);
                    
                    free(json_str);
                    cJSON_Delete(root);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void init_buttons() {
    char ptr_tx[16], ptr_rx[16], btn_cfg[1024];
    read_kiosk_config(ptr_tx, sizeof(ptr_tx), ptr_rx, sizeof(ptr_rx), btn_cfg, sizeof(btn_cfg));
    
    if (strlen(btn_cfg) > 0) {
        cJSON *root = cJSON_Parse(btn_cfg);
        if (root && cJSON_IsArray(root)) {
            g_button_count = cJSON_GetArraySize(root);
            if (g_button_count > MAX_BUTTONS) g_button_count = MAX_BUTTONS;
            
            for (int i=0; i<g_button_count; i++) {
                cJSON *item = cJSON_GetArrayItem(root, i);
                if (item) {
                    cJSON *pinObj = cJSON_GetObjectItem(item, "pin");
                    cJSON *svcObj = cJSON_GetObjectItem(item, "svc");
                    if (pinObj && svcObj) {
                        g_buttons[i].pin = pinObj->valueint;
                        strncpy(g_buttons[i].svc, svcObj->valuestring, sizeof(g_buttons[i].svc));
                        
                        // Configure GPIO
                        gpio_config_t io_conf = {};
                        io_conf.intr_type = GPIO_INTR_DISABLE;
                        io_conf.mode = GPIO_MODE_INPUT;
                        io_conf.pin_bit_mask = (1ULL << g_buttons[i].pin);
                        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
                        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
                        gpio_config(&io_conf);
                    }
                }
            }
        }
        cJSON_Delete(root);
    }
    
    xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
}

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

// ==================== WIFI EVENT HANDLER ====================
// Event handler for wifi and IP events
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        add_device_log("Connecting to AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            add_device_log("Retry connection to AP (%d/%d)", s_retry_num, MAXIMUM_RETRY);
        } else {
            add_device_log("WiFi connection failed after max retries.");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        char ip_str[32];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        add_device_log("Successfully got IP: %s", ip_str);
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
    if (len >= dst_max * 3) {
        len = dst_max * 3 - 1;
    }
    char raw[1024];
    if (len >= sizeof(raw)) len = sizeof(raw) - 1;
    strncpy(raw, ptr, len);
    raw[len] = '\0';
    url_decode(dst, raw);
    return true;
}


static esp_err_t kiosk_get_handler(httpd_req_t *req)
{
    char ptr_tx[16] = {0}, ptr_rx[16] = {0}, btn_cfg[1024] = {0};
    read_kiosk_config(ptr_tx, sizeof(ptr_tx), ptr_rx, sizeof(ptr_rx), btn_cfg, sizeof(btn_cfg));

    std::string html = kiosk_html_page;
    html = replace_placeholder(html, "{{PTR_TX}}", ptr_tx);
    html = replace_placeholder(html, "{{PTR_RX}}", ptr_rx);
    html = replace_placeholder(html, "{{BTN_CFG}}", btn_cfg);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t kiosk_post_handler(httpd_req_t *req)
{
    char body[1024] = {0};
    int ret, remaining = req->content_len;
    if (remaining >= sizeof(body)) remaining = sizeof(body) - 1;

    if ((ret = httpd_req_recv(req, body, remaining)) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    body[ret] = '\0';

    char ptr_tx[16] = {0}, ptr_rx[16] = {0}, btn_cfg[1024] = {0};
    parse_url_param(body, "ptr_tx", ptr_tx, sizeof(ptr_tx));
    parse_url_param(body, "ptr_rx", ptr_rx, sizeof(ptr_rx));
    parse_url_param(body, "btn_cfg", btn_cfg, sizeof(btn_cfg));

    save_kiosk_config(ptr_tx, ptr_rx, btn_cfg);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "<script>alert('Kiosk settings saved. Rebooting...'); setTimeout(()=>location.href='/kiosk', 3000);</script>", HTTPD_RESP_USE_STRLEN);
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
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
    html = replace_placeholder(html, "{{MQTT_SERVER}}", strlen(mqtt_server) > 0 ? mqtt_server : "qms1.camdvr.org");
    html = replace_placeholder(html, "{{MQTT_PORT}}", strlen(mqtt_port) > 0 ? mqtt_port : "1993");
    html = replace_placeholder(html, "{{MQTT_USER}}", strlen(mqtt_user) > 0 ? mqtt_user : "thom");
    html = replace_placeholder(html, "{{MQTT_PASS}}", strlen(mqtt_pass) > 0 ? mqtt_pass : "301258");
    html = replace_placeholder(html, "{{MQTT_TOPIC}}", strlen(mqtt_topic) > 0 ? mqtt_topic : "qms/display");
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
    int remaining = req->content_len;
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
    free(buf);

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
    
    httpd_resp_sendstr_chunk(req, success_html_1);
    httpd_resp_sendstr_chunk(req, NULL);

    xTaskCreate(restart_task, "restart_task", 2048, NULL, 5, NULL);

    return ESP_OK;
}

static esp_err_t log_get_handler(httpd_req_t *req)
{
    std::string html = log_page;
    html = replace_placeholder(html, "{{DEV_ID}}", g_dev_id);
    html = replace_placeholder(html, "{{DEV_KEY}}", g_dev_key);
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html.c_str(), html.length());
}

static esp_err_t log_data_get_handler(httpd_req_t *req)
{
    std::string all_logs = "";
    {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        for (const auto& log_line : g_device_logs) {
            all_logs += log_line + "\n";
        }
    }
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_send(req, all_logs.c_str(), all_logs.length());
}

static esp_err_t publish_post_handler(httpd_req_t *req)
{
    int remaining = req->content_len;
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
    char payload[512] = {0};

    parse_url_param(buf, "topic", topic, sizeof(topic));
    parse_url_param(buf, "payload", payload, sizeof(payload));
    free(buf);

    if (strlen(topic) > 0 && strlen(payload) > 0) {
        if (mqtt_client) {
            int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
            add_device_log("Web Publish: Topic='%s', MsgID=%d, Payload='%s'", topic, msg_id, payload);
            httpd_resp_sendstr(req, "SUCCESS");
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

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 16384; // Increase stack size to prevent stack overflow
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

        httpd_uri_t kiosk_get = {
            .uri       = "/kiosk",
            .method    = HTTP_GET,
            .handler   = kiosk_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &kiosk_get);

        httpd_uri_t kiosk_post = {
            .uri       = "/kiosk",
            .method    = HTTP_POST,
            .handler   = kiosk_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &kiosk_post);


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

        httpd_uri_t publish_post = {
            .uri       = "/publish",
            .method    = HTTP_POST,
            .handler   = publish_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &publish_post);

        return server;
    }

    ESP_LOGE(TAG, "Error starting web server!");
    return NULL;
}

extern "C" void app_main(void)
{
    init_printer();

    printQueue = xQueueCreate(50, sizeof(PrintTicketData));
    xTaskCreate(printer_task, "printer_task", 4096, NULL, 4, NULL);
    init_buttons();


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
    } else {
        add_device_log("No stored WiFi credentials found in NVS.");
    }

    if (!connected) {
        add_device_log("Starting Access Point and Captive Portal...");
        
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

        add_device_log("AP Mode started. SSID: %s", ap_ssid);
        add_device_log("Connect to AP and open http://192.168.4.1");

        start_webserver();
    } else {
        add_device_log("Starting Web Server in Station mode...");
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
            mqtt_app_start(mqtt_server, port, mqtt_user, mqtt_pass, mqtt_topic);
        } else {
            add_device_log("MQTT Broker not configured. Skipping MQTT connection.");
        }
    }
}
