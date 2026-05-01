#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize Wi-Fi subsystem
 * @return ESP_OK on success
 */
esp_err_t wifi_init(void);

/**
 * @brief Connect to Wi-Fi access point
 * @param ssid SSID to connect to
 * @param password Password for the AP
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t wifi_connect(const char *ssid, const char *password);

/**
 * @brief Disconnect from Wi-Fi
 * @return ESP_OK on success
 */
esp_err_t wifi_disconnect(void);

/**
 * @brief Check if Wi-Fi is connected
 * @return true if connected, false otherwise
 */
bool wifi_is_connected(void);

#endif // WIFI_MANAGER_H
