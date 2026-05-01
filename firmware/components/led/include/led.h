#pragma once
#include <stdbool.h>

void led_init(void);
void led_set(bool on);
void led_toggle(void);
void led_blink(int count, int period_ms);
