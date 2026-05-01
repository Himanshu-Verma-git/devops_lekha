#include "logger.h"
#include "esp_log.h"

static const char *TAG = "LOGGER";

void logger_init(void) { ESP_LOGI(TAG, "Logger Initialized (Dummy)"); }
