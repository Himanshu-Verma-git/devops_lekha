#include "audio_recorder.h"
#include "../../main/app_config.h"
#include "driver/i2s_pdm.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "AUDIO_REC";
static i2s_chan_handle_t rx_handle = NULL;

esp_err_t audio_recorder_init(void) {
  if (rx_handle)
    return ESP_OK;

  i2s_chan_config_t chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

  i2s_pdm_rx_config_t pdm_rx_cfg = {
      .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
      .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                 I2S_SLOT_MODE_MONO),
      .gpio_cfg =
          {
              .clk = I2S_MIC_SCK,
              .din = I2S_MIC_SD,
              .invert_flags =
                  {
                      .clk_inv = false,
                  },
          },
  };

  ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_rx_cfg));
  ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

  ESP_LOGI(TAG, "Audio Recorder Initialized (PDM RX)");
  return ESP_OK;
}

esp_err_t audio_recorder_start(void) {
  // Already enabled in init for simplicity, but could start/stop here
  // i2s_channel_enable(rx_handle);
  return ESP_OK;
}

esp_err_t audio_recorder_stop(void) {
  // i2s_channel_disable(rx_handle);
  return ESP_OK;
}

int audio_recorder_read(void *buf, size_t len) {
  size_t bytes_read = 0;
  if (rx_handle) {
    // Increase timeout to 1000ms to be safe, or handle ESP_ERR_TIMEOUT
    esp_err_t ret =
        i2s_channel_read(rx_handle, buf, len, &bytes_read, pdMS_TO_TICKS(1000));
    if (ret == ESP_OK) {
      return bytes_read;
    } else {
      if (ret == ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "I2S Read Timeout (got %d bytes)", bytes_read);
        return bytes_read; // Return what we got
      }
      ESP_LOGE(TAG, "I2S Read Failed: %s", esp_err_to_name(ret));
    }
  }
  return -1;
}

void audio_generate_wav_header(wav_header_t *header, uint32_t data_len,
                               uint32_t sample_rate, uint16_t channels,
                               uint16_t bits) {
  memcpy(header->riff_header, "RIFF", 4);
  header->wav_size = data_len + 36;
  memcpy(header->wave_header, "WAVE", 4);
  memcpy(header->fmt_header, "fmt ", 4);
  header->fmt_chunk_size = 16;
  header->audio_format = 1; // PCM
  header->num_channels = channels;
  header->sample_rate = sample_rate;
  header->byte_rate = sample_rate * channels * (bits / 8);
  header->block_align = channels * (bits / 8);
  header->bits_per_sample = bits;
  memcpy(header->data_header, "data", 4);
  header->data_bytes = data_len;
}
