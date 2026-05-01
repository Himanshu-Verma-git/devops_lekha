#include "api_client.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "esp_crt_bundle.h"

static const char *TAG = "API_CLIENT";

#define MAX_HTTP_OUTPUT_BUFFER 4096

static char http_response_buffer[MAX_HTTP_OUTPUT_BUFFER];
static int response_len = 0;

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ON_DATA:
    if (response_len + evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
      memcpy(http_response_buffer + response_len, evt->data, evt->data_len);
      response_len += evt->data_len;
      http_response_buffer[response_len] = 0;
    }
    break;
  default:
    break;
  }
  return ESP_OK;
}

esp_err_t api_client_init(void) {
  ESP_LOGI(TAG, "API Client initialized");
  return ESP_OK;
}

esp_err_t api_stt_transcribe(const char *wav_file_path, const char *api_key,
                             char *transcription, size_t transcription_size) {
  ESP_LOGI(TAG, "Transcribing: %s", wav_file_path);

  // Read WAV file
  FILE *f = fopen(wav_file_path, "rb");
  if (!f) {
    ESP_LOGE(TAG, "Failed to open WAV file");
    return ESP_FAIL;
  }

  struct stat st;
  stat(wav_file_path, &st);
  size_t file_size = st.st_size;

  #define MULTIPART_BOUNDARY "----WebKitFormBoundary7MA4YWxkTrZu0gW"
  const char *body_header = "--" MULTIPART_BOUNDARY "\r\n"
                            "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n"
                            "Content-Type: audio/wav\r\n\r\n";
  const char *body_footer = "\r\n--" MULTIPART_BOUNDARY "--\r\n";

  size_t body_header_len = strlen(body_header);
  size_t body_footer_len = strlen(body_footer);
  size_t payload_len = body_header_len + file_size + body_footer_len;

  response_len = 0;
  memset(http_response_buffer, 0, sizeof(http_response_buffer));

  esp_http_client_config_t config_http = {
      .url = "https://api.sarvam.ai/speech-to-text-translate",
      .method = HTTP_METHOD_POST,
      .timeout_ms = 30000,
      .crt_bundle_attach = esp_crt_bundle_attach,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config_http);
  char content_type[128];
  snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", MULTIPART_BOUNDARY);
  esp_http_client_set_header(client, "Content-Type", content_type);
  esp_http_client_set_header(client, "api-subscription-key", api_key);

  esp_err_t err = esp_http_client_open(client, payload_len);
  if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
      fclose(f);
      esp_http_client_cleanup(client);
      return err;
  }

  // 1. Write headers
  esp_http_client_write(client, body_header, body_header_len);

  // 2. Stream file
  uint8_t buffer[1024];
  size_t read_bytes;
  while ((read_bytes = fread(buffer, 1, sizeof(buffer), f)) > 0) {
      int wlen = esp_http_client_write(client, (const char *)buffer, read_bytes);
      if (wlen < 0) {
          ESP_LOGE(TAG, "Write failed");
          break;
      }
  }
  fclose(f);

  // 3. Write footer
  esp_http_client_write(client, body_footer, body_footer_len);

  // Fetch response
  int content_length = esp_http_client_fetch_headers(client);
  if (content_length >= 0) {
      // Reading response
      int read_len = esp_http_client_read(client, http_response_buffer, sizeof(http_response_buffer) - 1);
      if (read_len >= 0) {
          http_response_buffer[read_len] = 0;
      }

      int status = esp_http_client_get_status_code(client);
      ESP_LOGI(TAG, "STT HTTP Status: %d", status);

      if (status == 200) {
        cJSON *response = cJSON_Parse(http_response_buffer);
        if (response) {
          cJSON *transcript = cJSON_GetObjectItem(response, "transcript");
          if (transcript && transcript->valuestring) {
            strncpy(transcription, transcript->valuestring, transcription_size - 1);
            ESP_LOGI(TAG, "Transcription: %s", transcription);
          } else {
            ESP_LOGW(TAG, "Failed to extract 'transcript' from response");
          }
          cJSON_Delete(response);
        }
      } else {
        ESP_LOGE(TAG, "STT Error Response: %s", http_response_buffer);
        err = ESP_FAIL;
      }
  } else {
      ESP_LOGE(TAG, "HTTP client fetch headers failed");
      err = ESP_FAIL;
  }

  esp_http_client_cleanup(client);

  return err;
}

esp_err_t api_gemini_extract_tags(const char *transcription,
                                  const char *api_key, char *tags,
                                  size_t tags_size) {
  ESP_LOGI(TAG, "Extracting tags from: %s", transcription);

  const char *url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-3-flash-preview:generateContent";

  // Create prompt
  char prompt[512];
  snprintf(prompt, sizeof(prompt),
           "Extract 3-5 intent tags from this voice note. Return only "
           "comma-separated tags: \"%s\"",
           transcription);

  cJSON *root = cJSON_CreateObject();
  cJSON *contents = cJSON_CreateArray();
  cJSON *content = cJSON_CreateObject();
  cJSON *parts = cJSON_CreateArray();
  cJSON *part = cJSON_CreateObject();
  cJSON_AddStringToObject(part, "text", prompt);
  cJSON_AddItemToArray(parts, part);
  cJSON_AddItemToObject(content, "parts", parts);
  cJSON_AddItemToArray(contents, content);
  cJSON_AddItemToObject(root, "contents", contents);

  char *json_str = cJSON_PrintUnformatted(root);

  response_len = 0;
  memset(http_response_buffer, 0, sizeof(http_response_buffer));

  esp_http_client_config_t config_http = {
      .url = url,
      .method = HTTP_METHOD_POST,
      .event_handler = _http_event_handler,
      .timeout_ms = 30000,
      .crt_bundle_attach = esp_crt_bundle_attach,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config_http);
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_header(client, "x-goog-api-key", api_key);
  esp_http_client_set_post_field(client, json_str, strlen(json_str));

  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) {
    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Gemini HTTP Status: %d", status);

    if (status == 200) {
      cJSON *response = cJSON_Parse(http_response_buffer);
      if (response) {
        cJSON *candidates = cJSON_GetObjectItem(response, "candidates");
        if (candidates && cJSON_GetArraySize(candidates) > 0) {
          cJSON *first_candidate = cJSON_GetArrayItem(candidates, 0);
          cJSON *content = cJSON_GetObjectItem(first_candidate, "content");
          cJSON *parts = cJSON_GetObjectItem(content, "parts");
          if (parts && cJSON_GetArraySize(parts) > 0) {
            cJSON *first_part = cJSON_GetArrayItem(parts, 0);
            cJSON *text = cJSON_GetObjectItem(first_part, "text");
            if (text && text->valuestring) {
              strncpy(tags, text->valuestring, tags_size - 1);
              ESP_LOGI(TAG, "Tags: %s", tags);
            }
          }
        }
        cJSON_Delete(response);
      }
    } else {
      ESP_LOGE(TAG, "Gemini Error Response: %s", http_response_buffer);
    }
  } else {
    ESP_LOGE(TAG, "Gemini HTTP request failed: %s", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);
  free(json_str);
  cJSON_Delete(root);

  return err;
}

esp_err_t api_sheets_upload(const char *spreadsheet_id, const char *api_key,
                            const char *timestamp, const char *transcription,
                            const char *tags) {
  ESP_LOGI(TAG, "Uploading to Google Sheets via Web App");

  const char *url = "https://script.google.com/macros/s/AKfycbx1fzvzKGO7K0akiIvyWddSJv_-ZddPwMp_6PE-a6eMAF6FWEuKlJA6KbL24pySdAUfMg/exec";

  // Create simple flat JSON request expected by the Web App
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "timestamp", timestamp);
  cJSON_AddStringToObject(root, "transcription", transcription);
  cJSON_AddStringToObject(root, "tags", tags);

  char *json_str = cJSON_PrintUnformatted(root);

  response_len = 0;
  memset(http_response_buffer, 0, sizeof(http_response_buffer));

  esp_http_client_config_t config_http = {
      .url = url,
      .method = HTTP_METHOD_POST,
      .event_handler = _http_event_handler,
      .timeout_ms = 30000,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .disable_auto_redirect = true, // Do not follow redirects, as Google Sheets Web App responds with 302 to a GET-only page
  };

  esp_http_client_handle_t client = esp_http_client_init(&config_http);
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, json_str, strlen(json_str));

  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) {
    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Sheets HTTP Status: %d", status);
    // Any status 200-302 is considered successful for GAS redirects
    if (status == 200 || status == 302 || status == 301) {
      ESP_LOGI(TAG, "Successfully uploaded to Google Sheets");
    } else {
      ESP_LOGE(TAG, "Sheets Unexpected Status: %s", http_response_buffer);
      err = ESP_FAIL;
    }
  } else {
    ESP_LOGE(TAG, "Sheets HTTP request failed: %s", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);
  free(json_str);
  cJSON_Delete(root);

  return err;
}
