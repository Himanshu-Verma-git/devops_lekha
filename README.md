# Lekha - Intelligent Voice Activity Tracker

Lekha is an  activity tracking system that captures audio at the edge, transmits it securely via MQTT, and leverages advanced AI for speech-to-text (STT) and intelligent categorization.

## 🚀 Intent
The primary goal of Lekha is to provide a seamless, hands-free way to track activities. By using an ESP32-based edge device, users can record voice notes which are automatically transcribed and tagged with relevant categories (e.g., Work, Personal, Fitness) using AI, making activity logging effortless.

## 🛠 Tech Stack
- **Edge (Firmware):** ESP32-S3, ESP-IDF, MQTT (Audio streaming)
- **Infrastructure:** AWS (EKS, S3, SQS, RDS), Terraform, Kubernetes
- **Services:** Python, Docker, OpenAI/Whisper (for STT), LangChain (for intent/tags)
- **Messaging:** MQTT Broker (Mosquitto/EMQX)
- **Frontend:** Node.js, Express, Vanilla CSS (Premium Design)

## 🔄 Workflow
1. **Capture:** The ESP32 edge device records audio and streams it in real-time or chunks to the MQTT broker.
2. **Ingest:** The `mqtt-consumer` service subscribes to the audio topics, uploads the raw audio to **Amazon S3**, and pushes a processing task to **Amazon SQS**.
3. **Process:** The `stt-worker` polls SQS, downloads the audio from S3, and performs Speech-to-Text.
4. **Analyze:** The `intent-service` takes the transcript and uses AI to generate tags and categorize the activity.
5. **Store:** All metadata, transcripts, and tags are stored in a **PostgreSQL (RDS)** database.
6. **Visualize:** The web dashboard fetches data via the `backend-api` and displays it to the user.

## 🚀 Deployment Process

Follow these steps to deploy the complete Lekha system to AWS.

### Step 1: Cloud Infrastructure (Terraform)
Provision the core resources including EKS, S3, SQS, and RDS.
1. Navigate to the Terraform production directory:
   ```bash
   cd infra/terraform/environments/prod
   ```
2. Initialize and apply:
   ```bash
   terraform init
   terraform apply -var-file="secrets.tfvars"
   ```
3. **IMPORTANT:** After a successful apply, note down the following outputs:
   - `sqs_url`
   - `s3_bucket_name`
   - `rds_endpoint` (PostgreSQL URL)

### Step 2: Build & Push Docker Images
Build the containers for the microservices and push them to your registry (e.g., Docker Hub or ECR).
```bash
# Build STT Worker
docker build -t your-registry/lekha-stt-worker:latest ./services/stt-worker
docker push your-registry/lekha-stt-worker:latest

# Build MQTT Consumer
docker build -t your-registry/lekha-mqtt-consumer:latest ./services/mqtt-consumer
docker push your-registry/lekha-mqtt-consumer:latest

# Build Frontend
docker build -t your-registry/lekha-frontend:latest ./frontend
docker push your-registry/lekha-frontend:latest
```

### Step 3: Update Kubernetes Configuration
Before deploying to Kubernetes, you MUST update the environment variables in the deployment manifests with the values obtained from the Terraform output in Step 1.

1. Open the following files:
   - `infra/k8s/base/stt-worker.yaml`
   - `infra/k8s/base/mqtt-consumer.yaml`
2. Locate the `env` section and update these values:
   - `S3_BUCKET_NAME`: Set to your new S3 bucket name.
   - `SQS_QUEUE_URL`: Set to your new SQS FIFO queue URL.
   - `POSTGRES_URL`: Set to your RDS endpoint (e.g., `postgresql://user:pass@host:5432/db`).

### Step 4: Deploy to Kubernetes
Apply the manifests to your EKS cluster.
1. Ensure your `kubectl` context is configured for the cluster:
   ```bash
   aws eks update-kubeconfig --region ap-south-1 --name lekha-cluster
   ```
2. Apply the base manifests:
   ```bash
   kubectl apply -f infra/k8s/base/
   ```
3. Verify that all pods are running:
   ```bash
   kubectl get pods -w
   ```

## 💻 Pseudo Code
### Edge Device (Firmware)
```python
while True:
    if record_button_pressed():
        audio_data = capture_mic_input()
        mqtt.publish("lekha/audio", audio_data)
```

### Processing Pipeline (Services)
```python
# Consumer
def on_mqtt_message(payload):
    s3_url = upload_to_s3(payload)
    sqs.send_message({"url": s3_url})

# STT Worker
def process_task(message):
    audio = download_from_s3(message.url)
    text = speech_to_text(audio)
    tags = analyze_intent(text)
    db.save({"transcript": text, "tags": tags})
```

## 🌐 System Connection
```mermaid
graph TD
    A[ESP32 Edge Device] -- MQTT --> B[MQTT Broker]
    B -- Subscribe --> C[MQTT Consumer Service]
    C -- Upload --> D[(Amazon S3)]
    C -- Notify --> E[Amazon SQS]
    E -- Poll --> F[STT Worker]
    F -- Download --> D
    F -- Categorize --> G[Intent Service]
    G -- Store --> H[(PostgreSQL RDS)]
    I[Web Frontend] -- Request --> J[Backend API]
    J -- Query --> H
```
