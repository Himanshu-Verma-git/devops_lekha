# Lekha Testing Suite

This directory contains tools and scripts for testing the Lekha ecosystem without the need for physical hardware.

## 🧪 Mocking the Edge Device
The primary testing tool is the **Mock Edge Device**. It simulates the behavior of the ESP32-S3 firmware by sending audio data over MQTT to the backend processing pipeline.

### Why use a Mock Device?
- **Hardware-less Development:** Developers can test the entire cloud pipeline (MQTT -> S3 -> SQS -> STT -> DB) without needing an ESP32 device.
- **Reproducibility:** Test with specific, high-quality audio recordings to verify STT accuracy and intent classification.
- **CI/CD Integration:** The mock device is containerized, allowing it to be run as part of an automated testing pipeline.

### How it Works
1. The mock device connects to the same MQTT broker used by the production system.
2. It reads `.wav` files from a local directory (`my_wavs/`).
3. It publishes the raw binary data of these files to the topic `audio/[DEVICE_ID]`, mimicking the firmware's transmission logic.
4. The backend services treat this data identically to data coming from a real device.

## 📂 Structure
- **`mock-edge-device/`**: Contains the Dockerized Python script that performs the MQTT publishing.
  - `mock_device.py`: Core logic for MQTT connection and file streaming.
  - `generate_test_audio.py`: A utility to create sample `.wav` files for testing.
  - `Dockerfile`: Used to package the mock device for deployment.

## 🚀 Quick Start
To start testing, follow the instructions in the [Mock Edge Device README](./mock-edge-device/README.md).

```bash
cd mock-edge-device
docker build -t lekha-mock-device .
docker run --env-file .env.test -v $(pwd)/my_wavs:/app/wav_files lekha-mock-device
```
