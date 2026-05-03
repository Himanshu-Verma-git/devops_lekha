# Lekha Backend Services

This directory contains the microservices that power the Lekha processing pipeline. Each service is containerized and designed to run on Kubernetes.

## 🧱 Microservices Overview

### 1. `mqtt-consumer`
- **Role:** Bridges the Edge device and the Cloud.
- **Function:** Subscribes to MQTT audio topics, streams data to **Amazon S3**, and enqueues a processing task into **Amazon SQS**.
- **Stack:** Python, MQTT Client, Boto3.

### 2. `stt-worker`
- **Role:** Audio Processing.
- **Function:** Consumes messages from SQS, downloads audio from S3, and converts speech to text using AI models (Whisper/OpenAI).
- **Stack:** Python, PyTorch/Whisper, Boto3.

### 3. `intent-service`
- **Role:** Intelligence Layer.
- **Function:** Analyzes transcripts to identify user intent, extract keywords, and apply activity tags (categorization).
- **Stack:** Python, LangChain, OpenAI GPT.

### 4. `backend-api`
- **Role:** Data Provider.
- **Function:** Serves as the interface for the Frontend. It queries the RDS database to provide activity logs and analytics.
- **Stack:** Python (FastAPI/Flask), SQLAlchemy.

## 🚀 Deployment
Services are deployed as Docker containers. Use the provided `Dockerfile` in each subdirectory to build the images:
```bash
docker build -t lekha-[service-name] ./[service-directory]
```

## 🛠 Shared Tech Stack
- **Language:** Python 3.10+
- **Containerization:** Docker
- **Cloud Integration:** AWS SDK (Boto3)
- **Database:** PostgreSQL (via SQLAlchemy)
