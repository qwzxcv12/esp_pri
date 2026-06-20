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

#define MAX_LOG_LINES 50

extern "C" void printMQTTTicket(const char* ticket, const char* counter, const char* service, const char* cust_name);

// Global variables (using inline to avoid multiple definition errors)
inline std::vector<std::string> g_device_logs;
inline std::mutex g_log_mutex;
inline esp_mqtt_client_handle_t mqtt_client = NULL;
inline char g_dev_id[128] = {0};
inline char g_dev_key[128] = {0};
inline char g_mqtt_topic[256] = {0};

static const char *MQTT_TAG = "mqtt_qms";

// Add log helper
inline void add_device_log(const char* format, ...) {
    char log_buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(log_buf, sizeof(log_buf), format, args);
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
                    
                    // Subscribe to device command topic
                    if (strlen(g_dev_id) > 0) {
                        char dev_topic[320];
                        snprintf(dev_topic, sizeof(dev_topic), "%s/%s/command", token, g_dev_id);
                        int msg_id2 = esp_mqtt_client_subscribe(client, dev_topic, 1);
                        add_device_log("Subscribed to dev: %s (msg_id=%d)", dev_topic, msg_id2);
                    }
                }
                token = strtok(NULL, ",");
            }
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
                                
                                printMQTTTicket(ticket ? ticket : "", 
                                                counter ? counter : "", 
                                                service ? service : "", 
                                                cust_name ? cust_name : "");
                            }
                        } else if (strcmp(cmd->valuestring, "clear_display") == 0) {
                            add_device_log(">>> COMMAND: Clear screen");
                        }
                    }
                    cJSON_Delete(root);
                } else {
                    add_device_log("Warning: Failed to parse JSON");
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

inline void mqtt_publish_message(const char* topic, const char* data) {
    if (mqtt_client) {
        esp_mqtt_client_publish(mqtt_client, topic, data, 0, 0, 0);
    }
}

inline void mqtt_app_start(const char* broker, int port, const char* user, const char* pass, const char* topic)
{
    char clean_host[128] = {0};
    clean_broker_host(clean_host, broker, sizeof(clean_host));
    
    // Copy topic to global variable for subscribe when connected
    strncpy(g_mqtt_topic, topic, sizeof(g_mqtt_topic) - 1);
    g_mqtt_topic[sizeof(g_mqtt_topic) - 1] = '\0';

    add_device_log("Starting MQTT Client to %s:%d (User: %s, Topics: %s)", clean_host, port, strlen(user) > 0 ? user : "None", g_mqtt_topic);

    char uri[256];
    snprintf(uri, sizeof(uri), "mqtt://%s:%d", clean_host, port);

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

    if (strlen(g_dev_id) > 0 && strlen(g_dev_key) > 0) {
        xTaskCreate(mqtt_heartbeat_task, "mqtt_hb", 4096, NULL, 5, NULL);
    } else {
        add_device_log("Warning: Device credentials not set, Heartbeat disabled.");
    }
}

#endif // MQTT_HANDLER_H
