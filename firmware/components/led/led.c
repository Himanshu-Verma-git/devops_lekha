#include "led.h"
#include "../../main/app_config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LED";
static bool led_disabled = false;

void led_init(void) {
  // Check if LED pin is same as SD CS (21)
  if (LED_GPIO == 21) {
    ESP_LOGW(
        TAG,
        "LED GPIO 21 conflicts with SD_CS. Disabling explicit LED control.");
    led_disabled = true;
    return;
  }

  gpio_config_t io_conf = {.pin_bit_mask = (1ULL << LED_GPIO),
                           .mode = GPIO_MODE_OUTPUT,
                           .pull_up_en = GPIO_PULLUP_DISABLE,
                           .pull_down_en = GPIO_PULLDOWN_DISABLE,
                           .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf);
  led_set(false);
  ESP_LOGI(TAG, "LED Initialized on GPIO %d", LED_GPIO);
}

void led_set(bool on) {
  if (led_disabled)
    return;

  if (LED_ACTIVE_LEVEL == 0) {
    gpio_set_level(LED_GPIO, on ? 0 : 1);
  } else {
    gpio_set_level(LED_GPIO, on ? 1 : 0);
  }
}

void led_toggle(void) {
  if (led_disabled)
    return;
  int level = gpio_get_level(LED_GPIO);
  gpio_set_level(LED_GPIO, !level);
}

void led_blink(int count, int period_ms) {
  if (led_disabled)
    return;
  for (int i = 0; i < count; i++) {
    led_set(true);
    vTaskDelay(pdMS_TO_TICKS(period_ms / 2));
    led_set(false);
    vTaskDelay(pdMS_TO_TICKS(period_ms / 2));
  }
}
