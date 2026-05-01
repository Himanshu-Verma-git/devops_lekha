#ifndef API_CLIENT_H
#define API_CLIENT_H

#include "esp_err.h"
#include <stddef.h>

/**
 * @brief Initialize API client
 * @return ESP_OK on success
 */
esp_err_t api_client_init(void);

/**
 * @brief Transcribe audio file using Google Speech-to-Text
 * @param wav_file_path Path to WAV file on SD card
 * @param api_key Google Cloud API key
 * @param transcription Output buffer for transcription
 * @param transcription_size Size of transcription buffer
 * @return ESP_OK on success
 */
esp_err_t api_stt_transcribe(const char *wav_file_path, const char *api_key,
                             char *transcription, size_t transcription_size);

/**
 * @brief Extract tags/intent from transcription using Gemini
 * @param transcription Input transcription text
 * @param api_key Gemini API key
 * @param tags Output buffer for tags
 * @param tags_size Size of tags buffer
 * @return ESP_OK on success
 */
esp_err_t api_gemini_extract_tags(const char *transcription,
                                  const char *api_key, char *tags,
                                  size_t tags_size);

/**
 * @brief Upload data to Google Sheets
 * @param spreadsheet_id Google Sheets ID
 * @param api_key Google Sheets API key
 * @param timestamp Timestamp string
 * @param transcription Transcription text
 * @param tags Tags/intent text
 * @return ESP_OK on success
 */
esp_err_t api_sheets_upload(const char *spreadsheet_id, const char *api_key,
                            const char *timestamp, const char *transcription,
                            const char *tags);

#endif // API_CLIENT_H
