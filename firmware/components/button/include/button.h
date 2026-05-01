#pragma once

typedef enum {
  BUTTON_EVENT_PRESSED,
  BUTTON_EVENT_RELEASED,
  BUTTON_EVENT_LONG_PRESSED,
  BUTTON_EVENT_DOUBLE_CLICK
} button_event_t;

typedef void (*button_callback_t)(button_event_t event);

void button_init(button_callback_t cb);
