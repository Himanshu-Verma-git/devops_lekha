#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#define AUDIO_SAMPLE_RATE 16000
#define AUDIO_BIT_DEPTH 16
#define AUDIO_CHANNELS 1

typedef struct {
  char riff_header[4];     // "RIFF"
  uint32_t wav_size;       // File size - 8
  char wave_header[4];     // "WAVE"
  char fmt_header[4];      // "fmt "
  uint32_t fmt_chunk_size; // 16
  uint16_t audio_format;   // 1 for PCM
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;   // SampleRate * NumChannels * BitsPerSample/8
  uint16_t block_align; // NumChannels * BitsPerSample/8
  uint16_t bits_per_sample;
  char data_header[4]; // "data"
  uint32_t data_bytes; // Data size
} __attribute__((packed)) wav_header_t;

esp_err_t audio_recorder_init(void);
esp_err_t audio_recorder_start(void);
esp_err_t audio_recorder_stop(void);
int audio_recorder_read(void *buf, size_t len);
void audio_generate_wav_header(wav_header_t *header, uint32_t data_len,
                               uint32_t sample_rate, uint16_t channels,
                               uint16_t bits);
