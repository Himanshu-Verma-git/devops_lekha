#include "sd_card.h"
#include "../../main/app_config.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

static const char *TAG = "SD_CARD";
static bool is_mounted = false;
static sdmmc_card_t *card;
static spi_host_device_t host_id = SPI2_HOST;

#define MOUNT_POINT "/sdcard"

esp_err_t sd_card_init(void) {
  if (is_mounted)
    return ESP_OK;

  esp_err_t ret;
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = true,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  ESP_LOGI(TAG, "Initializing SD card via SPI...");
  ESP_LOGI(TAG, "CS: %d, MOSI: %d, MISO: %d, CLK: %d", SD_SPI_CS, SD_SPI_MOSI,
           SD_SPI_MISO, SD_SPI_CLK);

  // SPI Bus Config
  spi_bus_config_t bus_cfg = {
      .mosi_io_num = SD_SPI_MOSI,
      .miso_io_num = SD_SPI_MISO,
      .sclk_io_num = SD_SPI_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4096,
  };

  ret = spi_bus_initialize(host_id, &bus_cfg, SDSPI_DEFAULT_DMA);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SPI bus.");
    return ret;
  }

  // SD Host Config
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = host_id;
  host.max_freq_khz = SDMMC_FREQ_DEFAULT; // 20MHz

  // Slot Config
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = SD_SPI_CS;
  slot_config.host_id = host_id;

  ESP_LOGI(TAG, "Mounting filesystem...");
  ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config,
                                &card);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount filesystem.");
    } else {
      ESP_LOGE(TAG, "Failed to initialize the card (%s).",
               esp_err_to_name(ret));
    }
    // Deinit bus to allow retry?
    spi_bus_free(host_id);
    return ret;
  }

  sdmmc_card_print_info(stdout, card);
  ESP_LOGI(TAG, "SD Card mounted at %s", MOUNT_POINT);
  is_mounted = true;
  return ESP_OK;
}

void sd_card_deinit(void) {
  if (is_mounted) {
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    spi_bus_free(host_id);
    ESP_LOGI(TAG, "SD Card unmounted");
    is_mounted = false;
  }
}

bool sd_card_is_mounted(void) { return is_mounted; }
