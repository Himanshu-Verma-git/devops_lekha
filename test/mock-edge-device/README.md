# Mock ESP Edge Device

This directory contains a mock implementation of the ESP edge device, which is packaged as a Docker container. It reads `.wav` files from a mounted directory and publishes them to the configured MQTT broker. This is primarily used to test the backend architecture without relying on physical hardware.

## Setup

1. **Build the Docker Image:**
   ```bash
   docker build -t lekha-mock-device .
   ```

2. **Generate Test Audio:**
   If you don't have a `.wav` file, generate a test audio file using the provided Python script:
   ```bash
   python generate_test_audio.py
   ```
   This will create a `my_wavs/test_audio.wav` file containing a simple 3-second sine wave.

3. **Run the Container:**
   You need to provide your MQTT broker URL via an environment variable. You can pass it directly or use an `.env` file.

   Create an `.env` file containing your AWS broker details (e.g., `.env.test`):
   ```env
   MQTT_BROKER_URL=your-aws-eks-broker-url
   MQTT_PORT=1883
   DEVICE_ID=mock_device_001
   ```

   Run the container, mounting your `.wav` files directory to `/app/wav_files` inside the container:
   ```bash
   docker run --env-file .env.test -v $(pwd)/my_wavs:/app/wav_files lekha-mock-device
   ```

## Verification
- Monitor the output logs of the `mqtt-consumer` service on your AWS EKS cluster.
- You should see messages indicating an audio payload was received on `audio/mock_device_001`.
- Verify the file is stored in your S3 bucket under the `mock_device_001/` prefix.
- Verify the task was enqueued to SQS.
