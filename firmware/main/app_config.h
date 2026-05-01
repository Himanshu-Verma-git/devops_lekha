#pragma once

#include "driver/gpio.h"

// Hardware Pin Definitions for Seeed Studio XIAO ESP32S3 Sense

// LED (GPIO 21 is also SD CS. LED will flicker with SD activity. Explicit
// control disabled to prevent conflict)
#define LED_GPIO GPIO_NUM_21
#define LED_ACTIVE_LEVEL 0

// Button
#define BUTTON_GPIO GPIO_NUM_0
#define BUTTON_ACTIVE_LEVEL 0

// I2S Microphone
#define I2S_MIC_SCK GPIO_NUM_42
#define I2S_MIC_WS                                                             \
  GPIO_NUM_41 // LRCLK/WS often needed for I2S, but PDM uses just CLK/DAT
              // usually? Update: XIAO Sense Mic is PDM? Seeed Wiki:
              // "Microphone: MSM261D3526H1CPM" (I2S compatible). "Connected to
              // D1/D0?" No, "GPIO 41 (Data), GPIO 42 (Clock)".
#define I2S_MIC_SD GPIO_NUM_41

// SD Card (SDSPI) for XIAO ESP32S3 Sense Expansion Board
#define SD_SPI_CS GPIO_NUM_21
#define SD_SPI_CLK GPIO_NUM_7
#define SD_SPI_MISO GPIO_NUM_8
#define SD_SPI_MOSI GPIO_NUM_9
