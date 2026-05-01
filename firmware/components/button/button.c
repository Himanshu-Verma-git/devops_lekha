#include "button.h"
#include "../../main/app_config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "BUTTON";
static button_callback_t message_cb = NULL;

#define DEBOUNCE_TIME_MS 50
#define LONG_PRESS_TIME_MS 1000
#define DOUBLE_CLICK_TIME_MS 300

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void button_task(void *arg) {
  uint32_t io_num;
  int last_level = 1;
  // TickType_t last_time = 0;

  // State machine variables for double click / long press
  // For now, implementing simple Press/Release/Hold logic.
  // The requirement says:
  // - Press & Hold -> Start Recording (fired immediately on press? or after
  // hold?) "Records speech while a button is pressed and held." -> Implies
  // immediate start on press, stop on release. "Allows cancellation via double
  // press within 10 seconds."

  // So we need:
  // 1. Immediate PRESS event.
  // 2. RELEASE event.
  // DOUBLE CLICK might be handled by the logic: Press -> Release -> Press again
  // quickly.

  // Let's just report Raw Press/Release to logic, and maybe helper for double
  // click? Actually, double click detection usually requires delaying the
  // "Single Press" event, which is bad for "Press and hold to record"
  // (immediate).

  // Strategy:
  // Report PRESSED immediately.
  // Report RELEASED immediately.
  // Logic layer handles the "Cancel" check (if a second press comes in).

  // Wait, "Cancel via double press within 10 seconds OF RECORDING COMPLETION".
  // This means the Double Press happens *after* the first recording session
  // (Press...Release). So the sequence is:
  // 1. Press (Start Rec)
  // 2. Release (Stop Rec, Start Grace Period)
  // 3. User Presses again briefly? Or user Double Clicks?

  // IF "Double press within 10 seconds":
  // Usually means: Click... Click.

  // So the driver should just report Press and Release faithfully with
  // debouncing.

  while (1) {
    if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
      int level = gpio_get_level(io_num);
      if (level != last_level) {
        // Debounce
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
        int level_after = gpio_get_level(io_num);
        if (level_after == level) {
          last_level = level;

          if (level == BUTTON_ACTIVE_LEVEL) {
            if (message_cb)
              message_cb(BUTTON_EVENT_PRESSED);
          } else {
            if (message_cb)
              message_cb(BUTTON_EVENT_RELEASED);
          }
        }
      }
    }
  }
}

void button_init(button_callback_t cb) {
  message_cb = cb;

  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << BUTTON_GPIO),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en =
          (BUTTON_ACTIVE_LEVEL == 0) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
      .pull_down_en = (BUTTON_ACTIVE_LEVEL == 1) ? GPIO_PULLDOWN_ENABLE
                                                 : GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_ANYEDGE};
  gpio_config(&io_conf);

  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
  xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void *)BUTTON_GPIO);

  ESP_LOGI(TAG, "Button Initialized on GPIO %d", BUTTON_GPIO);
}
