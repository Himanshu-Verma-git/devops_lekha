# Deployment Guide for Lekha

This document outlines the steps to deploy the Lekha Voice Logger architecture and the ESP32S3 firmware.

## Architecture Deployment

### 1. Infrastructure Provisioning (Terraform)
The infrastructure is defined in `infra/terraform/environments/prod`.

**Secrets to provide:**
- `AWS_ACCESS_KEY_ID` and `AWS_SECRET_ACCESS_KEY`: Configured in your shell environment.
- `db_password`: The master password for the RDS PostgreSQL instance (pass via `-var` or a `.tfvars` file).

**Deployment Steps:**
```bash
cd infra/terraform/environments/prod
terraform init
terraform apply -var="db_password=your_secure_password"
```

**Outputs to save/export:**
- `s3_bucket_name`: The name of the S3 bucket for audio storage.
- `sqs_queue_url`: The URL of the SQS FIFO queue for task management.
- `rds_endpoint`: The host and port of the PostgreSQL database.
- `website_ip`: The public IP of the frontend EC2 instance.

### 2. Kubernetes Services (Backend)
The backend services (`mqtt-consumer`, `stt-worker`) and the `mqtt-broker` are deployed on EKS.

**MQTT Broker Setup:**
- **Type**: `LoadBalancer` Service.
- **Image**: `eclipse-mosquitto:2.0`.
- **Authentication**: Currently configured for **anonymous access**. To add authentication, you must modify the `command` in `infra/k8s/base/mqtt-broker.yaml` to include a `password_file` and set `allow_anonymous false`.

**Deployment Steps:**
1. Connect to your EKS cluster:
   ```bash
   aws eks update-kubeconfig --region <your-region> --name <your-cluster-name>
   ```
2. Build and push your service images (if not already done):
   - `lekha-mqtt-consumer:latest`
   - `lekha-stt-worker:latest`
3. Apply the Kubernetes manifests:
   ```bash
   kubectl apply -f infra/k8s/base/
   ```
4. Retrieve the MQTT Broker's external address:
   ```bash
   kubectl get svc mqtt-broker-svc
   ```
   Save the `EXTERNAL-IP`. This is your `MQTT_BROKER_HOST`.

### 3. Frontend Deployment
The frontend is a Node.js application running on a standalone EC2 instance.

**Secrets/Environment Variables:**
- `POSTGRES_URL`: `postgresql://lekhaadmin:<db_password>@<rds_endpoint>/lekhadb`

**Deployment Steps:**
1. SSH into the EC2 instance using the `website_ip`.
2. Deploy the `frontend/` directory and start the server:
   ```bash
   npm install
   POSTGRES_URL="your_url_here" node server.js
   ```

---

## Firmware Flashing

The firmware is designed for the ESP32S3 and uses ESP-IDF.

### Secrets to Provide
Create or edit `firmware/main/secrets.h` (based on `secrets.h.example`):

| Constant | Description |
| :--- | :--- |
| `WIFI_SSID` | Your local Wi-Fi network name. |
| `WIFI_PASSWORD` | Your local Wi-Fi password. |
| `MQTT_BROKER_URI` | The full URI: `mqtt://<MQTT_BROKER_HOST>:1883`. |

### Flashing Steps
1. Ensure ESP-IDF is installed and configured in your shell.
2. Navigate to the firmware directory:
   ```bash
   cd firmware
   idf.py build
   idf.py -p <YOUR_PORT> flash monitor
   ```

---

## Summary of Integration Secrets

| Source | Destination | Secret/Value |
| :--- | :--- | :--- |
| Terraform Output | K8s Services | `S3_BUCKET_NAME`, `SQS_QUEUE_URL` |
| Terraform Output | K8s/Frontend | `RDS_ENDPOINT` (used in `POSTGRES_URL`) |
| K8s Service | Firmware | `MQTT_BROKER_HOST` |
| User Provided | Firmware | `WIFI_SSID`, `WIFI_PASSWORD` |
| User Provided | Infra | `db_password` |


#
The POSTGRES_URL is constructed using values from your Terraform deployment. Here is exactly how to get each piece:

1. Get the <rds_endpoint>
After you run terraform apply, you can retrieve the database endpoint by running:

bash
cd infra/terraform/environments/prod
terraform output rds_endpoint
This will return something like: lekha-db.xxxxxxxxx.ap-south-1.rds.amazonaws.com:5432.

2. Construction Breakdown
Combine the values as follows:

Part	Source	Value
Protocol	Static	postgresql://
Username	

main.tf
lekhaadmin
Password	Your Input	The password you provided to -var="db_password=..."
Endpoint	Terraform Output	The value from terraform output rds_endpoint
Database	

main.tf
lekhadb
Example
If your endpoint is lekha-db.abc123.ap-south-1.rds.amazonaws.com:5432 and your password is MySecretPassword123, the final URL would be:

postgresql://lekhaadmin:MySecretPassword123@lekha-db.abc123.ap-south-1.rds.amazonaws.com:5432/lekhadb

TIP

You can also automate the construction of this URL in your shell after deployment:

bash
export RDS_EP=$(terraform output -raw rds_endpoint)
export POSTGRES_URL="postgresql://lekhaadmin:your_password@$RDS_EP/lekhadb"
#