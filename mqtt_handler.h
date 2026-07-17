#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <mutex>
#include <stdarg.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"

#define MAX_LOG_LINES 100

// Global variables (using inline to avoid multiple definition errors)
inline std::vector<std::string> g_device_logs;
inline std::mutex g_log_mutex;
inline esp_mqtt_client_handle_t mqtt_client = NULL;
inline char g_dev_id[128] = {0};
inline char g_dev_key[128] = {0};
inline char g_mqtt_topic[256] = {0};
inline char g_mqtt_server_host[128] = {0};

typedef struct {
    int id;
    char name[64];
    char prefix[16];
} qms_service_t;

inline qms_service_t g_services[10];
inline int g_service_count = 0;

#define MAX_PIN_MAPPINGS 20
typedef struct {
    int service_id;
    int pin;
} pin_mapping_t;

inline pin_mapping_t g_pin_mappings[MAX_PIN_MAPPINGS];
inline int g_pin_mapping_count = 0;
inline char g_board_type[16] = "ESP32"; // "ESP32" or "ESP32-S3"

static const char *MQTT_TAG = "mqtt_qms";

// Add log helper
inline __attribute__((noinline)) void add_device_log(const char* format, ...) {
    char *log_buf = (char *)malloc(1024);
    if (!log_buf) return;
    va_list args;
    va_start(args, format);
    vsnprintf(log_buf, 1024, format, args);
    va_end(args);

    // Get uptime
    uint32_t uptime = esp_log_timestamp() / 1000;
    uint32_t sec = uptime % 60;
    uint32_t min = (uptime / 60) % 60;
    uint32_t hour = (uptime / 3600);

    char time_str[32];
    snprintf(time_str, sizeof(time_str), "[%02d:%02d:%02d] ", (int)hour, (int)min, (int)sec);

    std::string full_log = std::string(time_str) + log_buf;

    std::lock_guard<std::mutex> lock(g_log_mutex);
    g_device_logs.push_back(full_log);
    if (g_device_logs.size() > MAX_LOG_LINES) {
        g_device_logs.erase(g_device_logs.begin());
    }
    
    // Also print to console
    ESP_LOGI(MQTT_TAG, "%s", log_buf);
    free(log_buf);
}

inline void clean_broker_host(char* dst, const char* src, size_t dst_len) {
    const char* start = src;
    const char* proto_end = strstr(src, "://");
    if (proto_end) {
        start = proto_end + 3;
    }
    size_t i = 0;
    while (*start && *start != '/' && *start != ':' && i < dst_len - 1) {
        dst[i++] = *start++;
    }
    dst[i] = '\0';
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        add_device_log("MQTT CONNECTED to broker!");

        if (strlen(g_mqtt_topic) > 0) {
            char topic_buf[256];
            strncpy(topic_buf, g_mqtt_topic, sizeof(topic_buf) - 1);
            topic_buf[sizeof(topic_buf) - 1] = '\0';
            
            char *token = strtok(topic_buf, ",");
            while (token != NULL) {
                // Trim leading spaces
                while (*token == ' ') token++;
                // Trim trailing spaces
                size_t len = strlen(token);
                while (len > 0 && token[len - 1] == ' ') {
                    token[len - 1] = '\0';
                    len--;
                }

                if (strlen(token) > 0) {
                    // Subscribe to base topic
                    int msg_id1 = esp_mqtt_client_subscribe(client, token, 1);
                    add_device_log("Subscribed to base: %s (msg_id=%d)", token, msg_id1);
                    
                    if (strlen(g_dev_id) > 0) {
                        char dev_topic[320];
                        snprintf(dev_topic, sizeof(dev_topic), "%s/%s/command", token, g_dev_id);
                        int msg_id2 = esp_mqtt_client_subscribe(client, dev_topic, 1);
                        add_device_log("Subscribed to dev: %s (msg_id=%d)", dev_topic, msg_id2);
                    }
                }
                token = strtok(NULL, ",");
            }
            
            // Also explicitly subscribe to qms/sender/.../command because backend hardcodes it for Kiosk config
            if (strlen(g_dev_id) > 0) {
                char sender_topic[320];
                snprintf(sender_topic, sizeof(sender_topic), "qms/sender/%s/command", g_dev_id);
                int msg_id_sender = esp_mqtt_client_subscribe(client, sender_topic, 1);
                add_device_log("Subscribed to sender cmd: %s (msg_id=%d)", sender_topic, msg_id_sender);
            }
        }
        
        // Request config after subscription
        if (strlen(g_dev_id) > 0) {
            char req_topic[320];
            snprintf(req_topic, sizeof(req_topic), "qms/sender/%s/request", g_dev_id);
            const char* req_payload = "{\"cmd\":\"get_config\"}";
            int msg_id3 = esp_mqtt_client_publish(client, req_topic, req_payload, 0, 1, 0);
            add_device_log("Sent get_config request: msg_id=%d", msg_id3);
        }
        break;

    case MQTT_EVENT_DISCONNECTED:
        add_device_log("MQTT DISCONNECTED. Reconnecting...");

        break;

    case MQTT_EVENT_SUBSCRIBED:
        add_device_log("MQTT Subscribed successfully, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        add_device_log("MQTT Unsubscribed, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        break;

    case MQTT_EVENT_DATA:
        add_device_log("MQTT Msg Rx on topic: %.*s", event->topic_len, event->topic);
        // Parse JSON
        {
            char *json_buf = (char*)malloc(event->data_len + 1);
            if (json_buf) {
                memcpy(json_buf, event->data, event->data_len);
                json_buf[event->data_len] = '\0';
                add_device_log("Payload: %s", json_buf);

                cJSON *root = cJSON_Parse(json_buf);
                bool processed = false;
                if (root) {
                    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
                    if (cmd && cJSON_IsString(cmd)) {
                        if (strcmp(cmd->valuestring, "display_ticket") == 0) {
                            cJSON *data = cJSON_GetObjectItem(root, "data");
                            if (data) {
                                const char *ticket = cJSON_GetStringValue(cJSON_GetObjectItem(data, "ticket"));
                                const char *counter = cJSON_GetStringValue(cJSON_GetObjectItem(data, "counter"));
                                const char *service = cJSON_GetStringValue(cJSON_GetObjectItem(data, "service"));
                                const char *color = cJSON_GetStringValue(cJSON_GetObjectItem(data, "color"));
                                const char *cust_name = cJSON_GetStringValue(cJSON_GetObjectItem(data, "cust_name"));
                                processed = true;
                                if (cust_name) {
                                    add_device_log(">>> CALLING: Ticket=%s, Counter=%s, Service=%s, Customer=%s", 
                                        ticket ? ticket : "N/A", 
                                        counter ? counter : "N/A", 
                                        service ? service : "N/A",
                                        cust_name);
                                } else {
                                    add_device_log(">>> CALLING: Ticket=%s, Counter=%s, Service=%s", 
                                        ticket ? ticket : "N/A", 
                                        counter ? counter : "N/A", 
                                        service ? service : "N/A");
                                }
                            }
                        } else if (strcmp(cmd->valuestring, "clear_display") == 0) {
                            add_device_log(">>> COMMAND: Clear screen");
                            processed = true;
                        } else if (strcmp(cmd->valuestring, "init_config") == 0) {
                            cJSON *unit_name_json = cJSON_GetObjectItem(root, "unit_name");
                            if (unit_name_json && unit_name_json->valuestring) {
                                add_device_log("Unit: %s", unit_name_json->valuestring);
                                printf("Unit: %s\n", unit_name_json->valuestring);
                            }

                            cJSON *services = cJSON_GetObjectItem(root, "services");
                            if (services && cJSON_IsArray(services)) {
                                g_service_count = 0;
                                int num_services = cJSON_GetArraySize(services);
                                add_device_log("=== DANH SÁCH DỊCH VỤ ===");
                                for (int i = 0; i < num_services && i < 10; i++) {
                                    cJSON *srv = cJSON_GetArrayItem(services, i);
                                    cJSON *id_json = cJSON_GetObjectItem(srv, "id");
                                    cJSON *name_json = cJSON_GetObjectItem(srv, "name");
                                    if (id_json && name_json) {
                                        g_services[g_service_count].id = id_json->valueint;
                                        strncpy(g_services[g_service_count].name, name_json->valuestring, sizeof(g_services[g_service_count].name) - 1);
                                        g_services[g_service_count].name[sizeof(g_services[g_service_count].name) - 1] = '\0';
                                        
                                        add_device_log("Nhấn phím %d: %s (Service ID: %d)", g_service_count + 1, g_services[g_service_count].name, g_services[g_service_count].id);
                                        printf("Nhấn phím %d: %s (Service ID: %d)\n", g_service_count + 1, g_services[g_service_count].name, g_services[g_service_count].id);
                                        g_service_count++;
                                    }
                                }
                                add_device_log("=========================");
                                printf("=========================\n");
                            }
                            processed = true;
                        }
                    }
                    cJSON_Delete(root);
                } else {
                    add_device_log("Warning: Failed to parse JSON");
                }
                if (!processed && strlen(json_buf) > 0) {
                }
                free(json_buf);
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        add_device_log("MQTT Error occurred");

        break;

    default:
        break;
    }
}

static void mqtt_heartbeat_task(void *pvParameters)
{
    char hb_topic[256];
    snprintf(hb_topic, sizeof(hb_topic), "qms/heartbeat/%s", g_dev_id);

    char hb_payload[256];
    snprintf(hb_payload, sizeof(hb_payload), "{\"secret_key\":\"%s\"}", g_dev_key);

    add_device_log("Heartbeat task active. Topic: %s", hb_topic);

    while (1) {
        if (mqtt_client) {
            int msg_id = esp_mqtt_client_publish(mqtt_client, hb_topic, hb_payload, 0, 0, 0);
            add_device_log("Heartbeat sent (msg_id=%d)", msg_id);
        }
        vTaskDelay(pdMS_TO_TICKS(20000));
    }
}

inline void mqtt_app_start(const char* broker, int port, const char* user, const char* pass, const char* topic)
{
    char clean_host[128] = {0};
    clean_broker_host(clean_host, broker, sizeof(clean_host));
    strncpy(g_mqtt_server_host, clean_host, sizeof(g_mqtt_server_host) - 1);
    g_mqtt_server_host[sizeof(g_mqtt_server_host) - 1] = '\0';
    
    // Copy topic to global variable for subscribe when connected
    strncpy(g_mqtt_topic, topic, sizeof(g_mqtt_topic) - 1);
    g_mqtt_topic[sizeof(g_mqtt_topic) - 1] = '\0';

    add_device_log("Starting MQTT Client to %s:%d (User: %s, Topics: %s)", clean_host, port, strlen(user) > 0 ? user : "None", g_mqtt_topic);

    char uri[256];
    snprintf(uri, sizeof(uri), "mqtt://%s:%d", clean_host, port);

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = uri;
#include "esp_idf_version.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    mqtt_cfg.buffer.size = 2048;
#else
    mqtt_cfg.buffer_size = 2048;
#endif
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

    if (strlen(g_dev_id) > 0 && strlen(g_dev_key) > 0) {
        xTaskCreate(mqtt_heartbeat_task, "mqtt_hb", 4096, NULL, 5, NULL);
    } else {
        add_device_log("Warning: Device credentials not set, Heartbeat disabled.");
    }
}

inline void send_ticket_request_by_service_id(int service_id) {
    if (mqtt_client && strlen(g_dev_id) > 0) {
        char req_topic[320];
        snprintf(req_topic, sizeof(req_topic), "qms/sender/%s/ticket_request", g_dev_id);
        
        char req_payload[128];
        snprintf(req_payload, sizeof(req_payload), "{\"service_id\": %d}", service_id);
        
        int msg_id = esp_mqtt_client_publish(mqtt_client, req_topic, req_payload, 0, 1, 0);
        add_device_log("Sent ticket_request (service_id=%d): msg_id=%d", service_id, msg_id);
        printf("Sent ticket_request (service_id=%d): msg_id=%d\n", service_id, msg_id);
    } else {
        add_device_log("Error: MQTT client not ready or device ID empty");
        printf("Error: MQTT client not ready or device ID empty\n");
    }
}

#endif // MQTT_HANDLER_H
