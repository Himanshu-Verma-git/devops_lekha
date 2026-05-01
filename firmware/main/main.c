#include "app_config.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

// Components
#include "audio_recorder.h"
#include "button.h"
#include "led.h"
#include "sd_card.h"
#include "secrets.h"
#include "time_sync.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "esp_mac.h"
#include "esp_system.h"

char device_mac_str[18] = {0};

static const char *TAG = "APP_MAIN";

static QueueHandle_t audio_cmd_queue = NULL;
static QueueHandle_t button_evt_queue = NULL;

typedef enum {
  CMD_START_REC,
  CMD_STOP_REC,
  CMD_SAVE_REC,
  CMD_CANCEL_REC
} audio_cmd_t;
static bool audio_save_done = false;

void button_callback(button_event_t event) {
  if (button_evt_queue) {
    xQueueSend(button_evt_queue, &event, 0);
  }
}

void audio_task(void *arg) {
  uint8_t *buffer = malloc(4096);
  FILE *f = NULL;
  bool is_recording = false;
  char temp_filename[] = "/sdcard/temp.wav";
  uint32_t total_bytes = 0;
  audio_cmd_t cmd;

  while (1) {
    if (xQueueReceive(audio_cmd_queue, &cmd, 0)) {
      if (cmd == CMD_START_REC && !is_recording) {
        f = fopen(temp_filename, "wb");
        if (f) {
          wav_header_t header;
          audio_generate_wav_header(&header, 0, AUDIO_SAMPLE_RATE,
                                    AUDIO_CHANNELS, AUDIO_BIT_DEPTH);
          fwrite(&header, sizeof(wav_header_t), 1, f);
          total_bytes = 0;
          is_recording = true;
          audio_recorder_start();
          ESP_LOGI(TAG, "Recording started");
        } else {
          ESP_LOGE(TAG, "Failed to open temp log file");
        }
      } else if (cmd == CMD_STOP_REC && is_recording) {
        is_recording = false;
        audio_recorder_stop();
        if (f) {
          rewind(f);
          wav_header_t header;
          audio_generate_wav_header(&header, total_bytes, AUDIO_SAMPLE_RATE,
                                    AUDIO_CHANNELS, AUDIO_BIT_DEPTH);
          fwrite(&header, sizeof(wav_header_t), 1, f);
          fclose(f);
          f = NULL;
          ESP_LOGI(TAG, "Recording stopped, total bytes: %lu", total_bytes);
        }
      } else if (cmd == CMD_SAVE_REC) {
        char timestamp[32];
        time_sync_get_timestamp(timestamp, sizeof(timestamp));
        char new_name[128];
        snprintf(new_name, sizeof(new_name), "/sdcard/%s.wav", timestamp);

        struct stat st;
        int counter = 1;
        while (stat(new_name, &st) == 0) {
          snprintf(new_name, sizeof(new_name), "/sdcard/%s_%d.wav", timestamp,
                   counter++);
        }

        if (rename(temp_filename, new_name) == 0) {
          ESP_LOGI(TAG, "Saved recording as %s", new_name);
        } else {
          ESP_LOGE(TAG, "Failed to save recording %s (errno: %d)", new_name,
                   errno);
        }
        audio_save_done = true;
      } else if (cmd == CMD_CANCEL_REC) {
        unlink(temp_filename);
        ESP_LOGI(TAG, "Recording cancelled and scrapped");
        audio_save_done = true;
      }
    }

    if (is_recording && f) {
      int len = audio_recorder_read(buffer, 1024);
      if (len > 0) {
        fwrite(buffer, 1, len, f);
        total_bytes += len;
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

void process_pending_files(void) {
  DIR *dir = opendir("/sdcard");
  bool files_to_process = false;
  if (dir) {
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
      if (strstr(ent->d_name, ".wav") && strcmp(ent->d_name, "temp.wav") != 0 &&
          strcmp(ent->d_name, "test.wav") != 0) {
        files_to_process = true;
        break;
      }
    }
    closedir(dir);
  }

  if (!files_to_process) {
    ESP_LOGI(TAG, "No files to process.");
    return;
  }

  ESP_LOGI(TAG, "Files marked for processing found! Attempting Wi-Fi.");
  wifi_init();

  bool wifi_connected = false;
  for (int i = 0; i < 10; i++) {
    ESP_LOGI(TAG, "Wi-Fi connection attempt %d/10", i + 1);
    if (wifi_connect(WIFI_SSID, WIFI_PASSWORD) == ESP_OK) {
      wifi_connected = true;
      break;
    }
  }

  if (wifi_connected) {
    ESP_LOGI(TAG, "Wi-Fi Connected! Syncing time and processing files.");
    time_sync_init();
    time_sync_wait(10000);

    // Initialize MQTT
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
    
    // Wait momentarily for connection to establish
    vTaskDelay(pdMS_TO_TICKS(2000));

    dir = opendir("/sdcard");
    if (dir) {
      struct dirent *ent;
      char *filepath = malloc(300);

      if (filepath) {
        while ((ent = readdir(dir)) != NULL) {
          if (strstr(ent->d_name, ".wav") &&
              strcmp(ent->d_name, "temp.wav") != 0 &&
              strcmp(ent->d_name, "test.wav") != 0) {
            snprintf(filepath, 300, "/sdcard/%s", ent->d_name);
            ESP_LOGI(TAG, "Processing %s...", filepath);

            FILE *f = fopen(filepath, "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                long fsize = ftell(f);
                fseek(f, 0, SEEK_SET);

                char *audio_data = malloc(fsize);
                if (audio_data) {
                    fread(audio_data, 1, fsize, f);
                    fclose(f);

                    char topic[64];
                    snprintf(topic, sizeof(topic), "audio/%s", device_mac_str);

                    int msg_id = esp_mqtt_client_publish(client, topic, audio_data, fsize, 1, 0);
                    ESP_LOGI(TAG, "Published audio to %s via MQTT. Msg ID: %d", topic, msg_id);

                    free(audio_data);

                    unlink(filepath);
                } else {
                    ESP_LOGE(TAG, "Failed to allocate memory for audio data");
                    fclose(f);
                }
            } else {
                ESP_LOGE(TAG, "Failed to open file %s", filepath);
            }
          }
        }
      } else {
        ESP_LOGE(TAG, "Failed to allocate memory for filepath");
      }

      if (filepath)
        free(filepath);

      closedir(dir);
    }
    
    // Allow messages to transmit
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_mqtt_client_stop(client);
    esp_mqtt_client_destroy(client);
    wifi_disconnect();
  } else {
    ESP_LOGW(TAG, "Unable to connect to Wi-Fi after 10 tries.");
  }
}

void app_main(void) {
  esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
  ESP_LOGI(TAG, "Wakeup cause: %d", wakeup_cause);

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(device_mac_str, sizeof(device_mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  ESP_LOGI(TAG, "Device MAC Address (Device ID): %s", device_mac_str);

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  led_init();
  if (sd_card_init() != ESP_OK) {
    ESP_LOGE(TAG, "SD Card Init Failed. Going to sleep.");
    goto enter_sleep;
  }

  audio_recorder_init();

  audio_cmd_queue = xQueueCreate(5, sizeof(audio_cmd_t));
  button_evt_queue = xQueueCreate(10, sizeof(button_event_t));
  xTaskCreate(audio_task, "audio_task", 8192, NULL, 5, NULL);
  button_init(button_callback);

  // 1. Check for Recording Action
  button_event_t evt;
  bool button_held = false;

  // We check the physical level directly, or wait briefly for an event
  if (gpio_get_level(BUTTON_GPIO) == BUTTON_ACTIVE_LEVEL) {
    button_held = true;
  } else if (wakeup_cause == ESP_SLEEP_WAKEUP_EXT0 ||
             wakeup_cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
    if (xQueueReceive(button_evt_queue, &evt, pdMS_TO_TICKS(800)) &&
        evt == BUTTON_EVENT_PRESSED) {
      button_held = true;
    }
  }

  if (button_held) {
    ESP_LOGI(TAG, "Button held! Starting recording logic.");

    audio_cmd_t cmd = CMD_START_REC;
    xQueueSend(audio_cmd_queue, &cmd, 0);
    led_set(true);

    int64_t start_time = esp_timer_get_time();
    while (1) {
      if (xQueueReceive(button_evt_queue, &evt, pdMS_TO_TICKS(10))) {
        if (evt == BUTTON_EVENT_RELEASED)
          break;
      }
      if (gpio_get_level(BUTTON_GPIO) != BUTTON_ACTIVE_LEVEL) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (gpio_get_level(BUTTON_GPIO) != BUTTON_ACTIVE_LEVEL)
          break;
      }
    }
    int64_t duration = (esp_timer_get_time() - start_time) / 1000;

    cmd = CMD_STOP_REC;
    xQueueSend(audio_cmd_queue, &cmd, 0);
    led_set(false);

    if (duration < 500) {
      ESP_LOGI(TAG, "Recording too short (%lld ms), cancelling.", duration);
      cmd = CMD_CANCEL_REC;
      xQueueSend(audio_cmd_queue, &cmd, 0);
    } else {
      ESP_LOGI(TAG, "Valid recording (%lld ms). Waiting 10s for cancel...",
               duration);
      bool cancelled = false;
      if (xQueueReceive(button_evt_queue, &evt, pdMS_TO_TICKS(10000))) {
        if (evt == BUTTON_EVENT_PRESSED) {
          cancelled = true;
        }
      }

      audio_save_done = false;
      if (cancelled) {
        ESP_LOGI(TAG, "Cancel requested via button press.");
        led_blink(2, 200);
        cmd = CMD_CANCEL_REC;
        xQueueSend(audio_cmd_queue, &cmd, 0);
      } else {
        ESP_LOGI(TAG, "Grace period ended, saving file.");
        cmd = CMD_SAVE_REC;
        xQueueSend(audio_cmd_queue, &cmd, 0);
      }
      while (!audio_save_done)
        vTaskDelay(1);
    }
  } else {
    ESP_LOGI(TAG, "No button action detected on boot.");
  }

  // Clear any extra button events from the queue so they don't leak into next
  // wake up
  xQueueReset(button_evt_queue);

  // 2. Processing Phase
  process_pending_files();

enter_sleep:
  // 3. Sleep Phase
  ESP_LOGI(TAG, "Entering Deep Sleep to conserve battery.");
  while (gpio_get_level(BUTTON_GPIO) == BUTTON_ACTIVE_LEVEL) {
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  esp_sleep_enable_ext0_wakeup(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL);
  esp_deep_sleep_start();
}
