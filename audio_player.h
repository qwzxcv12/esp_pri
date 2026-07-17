#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_http_client.h"
#include "esp_log.h"

#define I2S_SPEAKER_BCLK 33
#define I2S_SPEAKER_LRCK 32
#define I2S_SPEAKER_DOUT 2
#define I2S_SPEAKER_PORT I2S_NUM_1

static const char *AUDIO_TAG = "audio_player";

inline void init_i2s_audio() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
#ifdef I2S_COMM_FORMAT_STAND_I2S
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
#else
        .communication_format = I2S_COMM_FORMAT_I2S,
#endif
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SPEAKER_BCLK,
        .ws_io_num = I2S_SPEAKER_LRCK,
        .data_out_num = I2S_SPEAKER_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_SPEAKER_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(AUDIO_TAG, "Failed to install I2S driver: %d", err);
        return;
    }
    err = i2s_set_pin(I2S_SPEAKER_PORT, &pin_config);
    if (err != ESP_OK) {
        ESP_LOGE(AUDIO_TAG, "Failed to set I2S pins: %d", err);
        return;
    }
    i2s_zero_dma_buffer(I2S_SPEAKER_PORT);
    ESP_LOGI(AUDIO_TAG, "I2S Audio Initialized on GPIO %d, %d, %d", I2S_SPEAKER_BCLK, I2S_SPEAKER_LRCK, I2S_SPEAKER_DOUT);
}

struct tts_request_t {
    char text[128];
};

static void play_tts_task(void *pvParameters) {
    tts_request_t *req_data = (tts_request_t*)pvParameters;
    if (!req_data) {
        vTaskDelete(NULL);
        return;
    }

    char post_data[256];
    snprintf(post_data, sizeof(post_data), "{\"text\":\"%s\"}", req_data->text);

    // Hardcoded per user request
    const char* tts_url = "http://192.168.2.16/api/tts";

    esp_http_client_config_t config = {};
    config.url = tts_url;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 10000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        extern void add_device_log(const char* format, ...);
        add_device_log("TTS Error: Failed to init HTTP client");
        free(req_data);
        vTaskDelete(NULL);
        return;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    extern void add_device_log(const char* format, ...);
    add_device_log("TTS: Requesting speech: '%s'", req_data->text);
    add_device_log("TTS: Connecting to %s", tts_url);

    esp_err_t err = esp_http_client_open(client, strlen(post_data));
    if (err == ESP_OK) {
        int content_length = esp_http_client_fetch_headers(client);
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            add_device_log("TTS: Connected successfully! (Status: %d, Length: %d)", status_code, content_length);
            char buffer[1024];
            int read_bytes;
            bool header_skipped = false;

            while ((read_bytes = esp_http_client_read(client, buffer, sizeof(buffer))) > 0) {
                char *data_ptr = buffer;
                int data_len = read_bytes;

                if (!header_skipped) {
                    if (read_bytes > 44) {
                        uint32_t sample_rate = *(uint32_t*)(buffer + 24);
                        add_device_log("TTS: WAV Sample Rate: %d Hz", sample_rate);

                        i2s_set_sample_rates(I2S_SPEAKER_PORT, sample_rate);

                        data_ptr = buffer + 44;
                        data_len = read_bytes - 44;
                        header_skipped = true;
                    } else {
                        continue;
                    }
                }

                size_t bytes_written = 0;
                i2s_write(I2S_SPEAKER_PORT, data_ptr, data_len, &bytes_written, portMAX_DELAY);
            }
            vTaskDelay(pdMS_TO_TICKS(500));
            i2s_zero_dma_buffer(I2S_SPEAKER_PORT);
        } else {
            add_device_log("TTS Error: Invalid response length or status. Status: %d, Length: %d", status_code, content_length);
        }
    } else {
        add_device_log("TTS Error: Connect to %s failed (esp_err: %d)", tts_url, err);
    }

    esp_http_client_cleanup(client);
    free(req_data);
    vTaskDelete(NULL);
}

#endif // AUDIO_PLAYER_H
