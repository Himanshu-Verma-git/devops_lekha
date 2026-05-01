# Lekha Architecture Overview

Lekha is an event-driven voice logging system that captures audio from an ESP32S3 device, processes it using AI (STT and Intent Inference), and provides a web interface for viewing the results.

## System Components

### 1. Edge Device (ESP32S3)
- **Role**: Captures audio via a microphone.
- **Protocol**: MQTT.
- **Action**: Streams raw or compressed audio data to the central broker.

### 2. MQTT Broker (Mosquitto)
- **Role**: Central communication hub.
- **Deployment**: Hosted on EKS as a `LoadBalancer` service.
- **Security**: Supports anonymous or authenticated connections.

### 3. MQTT Consumer (Python)
- **Role**: Ingests audio streams from the broker.
- **Action**: Saves raw audio to **Amazon S3** and pushes a processing task to **Amazon SQS**.

### 4. STT Worker (Python / AI)
- **Role**: Heavy-duty processing.
- **Models**:
  - **Faster-Whisper**: For Speech-to-Text conversion.
  - **BART (Zero-Shot)**: For intent and tag extraction.
- **Action**: Polls SQS for tasks, downloads audio from S3, runs inference, and saves results to **Amazon RDS (PostgreSQL)**.

### 5. Frontend (Node.js/Express)
- **Role**: User interface.
- **Deployment**: Standalone EC2 instance.
- **Action**: Connects to RDS to display processed voice logs and manage user accounts.

## Data Flow
1. **Device** -> MQTT -> **Broker**
2. **Broker** -> **MQTT Consumer**
3. **MQTT Consumer** -> S3 (Store) & SQS (Queue)
4. **SQS** -> **STT Worker**
5. **STT Worker** -> S3 (Download) -> Inference -> **Postgres**
6. **Postgres** -> **Frontend** -> **User**
