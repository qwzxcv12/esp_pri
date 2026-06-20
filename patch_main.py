import os
import re

main_path = "main/main.cpp"

with open(main_path, "r", encoding="utf-8") as f:
    content = f.read()

# 1. Add headers
if "kiosk_config_html.h" not in content:
    content = content.replace('#include "thermal_printer.h"', 
        '#include "thermal_printer.h"\n#include "kiosk_config_html.h"\n#include "driver/gpio.h"\n#include "cJSON.h"\n')

# 2. Add NVS functions for kiosk config before init_printer
nvs_funcs = """
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
        btn_cfg[0] = '\\0';
        return err;
    }
    if (nvs_get_str(my_handle, "ptr_tx", ptr_tx, &t_len) != ESP_OK) strncpy(ptr_tx, "17", t_len);
    if (nvs_get_str(my_handle, "ptr_rx", ptr_rx, &r_len) != ESP_OK) strncpy(ptr_rx, "16", r_len);
    if (nvs_get_str(my_handle, "btn_cfg", btn_cfg, &b_len) != ESP_OK) btn_cfg[0] = '\\0';
    nvs_close(my_handle);
    return ESP_OK;
}
"""

if "save_kiosk_config" not in content:
    content = content.replace('ThermalPrinter* printer = NULL;', 
        nvs_funcs + '\nThermalPrinter* printer = NULL;')

# 3. Modify init_printer to use NVS
init_printer_old = """void init_printer() {
    uart_config_t uart_config = {};
    uart_config.baud_rate = 9600;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_APB;

    uart_driver_install(UART_NUM_2, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    printer = new ThermalPrinter(UART_NUM_2);
}"""

init_printer_new = """void init_printer() {
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
}"""
content = content.replace(init_printer_old, init_printer_new)


# 4. Print Queue Implementation
print_q_code = """
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
"""

# Replace old printMQTTTicket with Queue based one
old_print_func = re.search(r'extern "C" void printMQTTTicket\(.*?\}', content, re.DOTALL)
if old_print_func:
    content = content.replace(old_print_func.group(0), print_q_code)


# 5. Button Manager Implementation
btn_mgr_code = """
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
                    
                    extern void mqtt_publish_message(const char* topic, const char* data);
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
"""

if "init_buttons" not in content:
    # Insert before event_handler
    content = content.replace('static EventGroupHandle_t s_wifi_event_group;', btn_mgr_code + '\nstatic EventGroupHandle_t s_wifi_event_group;')


# 6. HTTP Handlers for /kiosk
kiosk_handlers = """
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
    body[ret] = '\\0';

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
"""

if "kiosk_get_handler" not in content:
    content = content.replace('static esp_err_t root_get_handler(httpd_req_t *req)', kiosk_handlers + '\nstatic esp_err_t root_get_handler(httpd_req_t *req)')


# 7. Register /kiosk in start_webserver
register_kiosk = """
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
"""

if "kiosk_get_handler(server" not in content and 'httpd_register_uri_handler(server, &kiosk_get);' not in content:
    content = content.replace('httpd_register_uri_handler(server, &root);', 
                              'httpd_register_uri_handler(server, &root);\n' + register_kiosk)


# 8. Start Queue and Tasks in app_main
app_main_add = """
    printQueue = xQueueCreate(10, sizeof(PrintTicketData));
    xTaskCreate(printer_task, "printer_task", 4096, NULL, 4, NULL);
    init_buttons();
"""

if "printQueue = xQueueCreate" not in content:
    content = content.replace('init_printer();', 'init_printer();\n' + app_main_add)


with open(main_path, "w", encoding="utf-8") as f:
    f.write(content)
print("main.cpp patched successfully!")
