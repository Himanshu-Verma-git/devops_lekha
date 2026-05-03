# Lekha Firmware - Edge Device

The Lekha firmware is designed to run on the ESP32-S3, providing high-quality audio capture and reliable streaming to the backend via MQTT.

## 🎙 Features
- **Audio Capture:** I2S microphone interface.
- **Streaming:** Real-time audio data transmission over MQTT.
- **Connectivity:** Wi-Fi managed with automatic reconnection logic.
- **Low Latency:** Optimized buffer management for smooth streaming.

## 🛠 Tech Stack
- **Framework:** ESP-IDF (v5.x)
- **Language:** C / C++
- **Communication:** MQTT (Eclipse Paho or ESP-MQTT)
- **Hardware:** ESP32-S3 (with support for external I2S Mic like INMP441)

## 🚀 Getting Started
1. **Prerequisites:** Install ESP-IDF and set up your environment.
2. **Configuration:**
   - Copy `main/secrets.h.example` to `main/secrets.h`.
   - Update `WIFI_SSID`, `WIFI_PASSWORD`, and `MQTT_BROKER_URI`.
3. **Build and Flash:**
   ```bash
   idf.py build
   idf.py -p [PORT] flash monitor
   ```

## 📂 Structure
- `main/`: Core logic for audio sampling and MQTT publishing.
- `components/`: Drivers for specific hardware components.
- `include/`: Global header files.
