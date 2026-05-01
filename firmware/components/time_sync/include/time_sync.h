#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize SNTP time synchronization
 * @return ESP_OK on success
 */
esp_err_t time_sync_init(void);

/**
 * @brief Wait for time to be synchronized
 * @param timeout_ms Maximum time to wait in milliseconds
 * @return ESP_OK if synced, ESP_ERR_TIMEOUT if timeout
 */
esp_err_t time_sync_wait(uint32_t timeout_ms);

/**
 * @brief Get formatted timestamp string
 * @param buffer Output buffer for timestamp
 * @param size Size of buffer (must be at least 16 bytes)
 * @return ESP_OK on success
 */
esp_err_t time_sync_get_timestamp(char *buffer, size_t size);

/**
 * @brief Check if time is synchronized
 * @return true if synced, false otherwise
 */
bool time_sync_is_synced(void);

#endif // TIME_SYNC_H
