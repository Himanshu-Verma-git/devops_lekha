#include "time_sync.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>

static const char *TAG = "TIME_SYNC";

static bool time_synced = false;

static void time_sync_notification_cb(struct timeval *tv) {
  ESP_LOGI(TAG, "Time synchronized");
  time_synced = true;
}

esp_err_t time_sync_init(void) {
  ESP_LOGI(TAG, "Initializing SNTP");

  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_setservername(1, "time.google.com");
  esp_sntp_setservername(2, "time.windows.com");
  sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
  esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
  esp_sntp_init();

  // Set timezone to IST (Kolkata/Delhi is UTC+5:30, so IST-5:30 in POSIX)
  setenv("TZ", "IST-5:30", 1);
  tzset();

  return ESP_OK;
}

esp_err_t time_sync_wait(uint32_t timeout_ms) {
  ESP_LOGI(TAG, "Waiting for time sync...");

  uint32_t elapsed = 0;
  while (!time_synced && elapsed < timeout_ms) {
    vTaskDelay(pdMS_TO_TICKS(100));
    elapsed += 100;
  }

  if (time_synced) {
    time_t now;
    time(&now);
    char strftime_buf[64];
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Current time: %s", strftime_buf);
    return ESP_OK;
  } else {
    ESP_LOGW(TAG, "Time sync timeout");
    return ESP_ERR_TIMEOUT;
  }
}

esp_err_t time_sync_get_timestamp(char *buffer, size_t size) {
  if (size < 16) {
    return ESP_ERR_INVALID_SIZE;
  }

  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  // Format: dd_mm_yy__hhmm
  snprintf(buffer, size, "%02d_%02d_%02d__%02d%02d", 
           timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year % 100, 
           timeinfo.tm_hour, timeinfo.tm_min);

  return ESP_OK;
}

bool time_sync_is_synced(void) { return time_synced; }
