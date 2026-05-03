# Lekha Infrastructure

This directory contains the Infrastructure as Code (IaC) definitions for the Lekha project, utilizing Terraform for cloud provisioning and Kubernetes for service orchestration.

## 🏗 Components
- **Terraform:** Manages AWS resources including EKS clusters, S3 buckets for audio storage, SQS queues for task management, and RDS for persistent storage.
- **Kubernetes (K8s):** Contains manifests for deploying services (API, Workers, Consumers) onto the EKS cluster.

## 🛠 Tech Stack
- **Cloud Provider:** AWS
- **Provisioning:** Terraform v1.x
- **Orchestration:** Amazon EKS (Kubernetes)
- **Database:** PostgreSQL (Amazon RDS)
- **Storage:** Amazon S3
- **Message Queue:** Amazon SQS

## 🚀 Setup & Deployment
1. **Initialize Terraform:**
   ```bash
   cd terraform/environments/prod
   terraform init
   ```
2. **Configure Secrets:**
   Copy `secrets.tfvars.example` to `secrets.tfvars` and fill in your credentials.
3. **Deploy Infrastructure:**
   ```bash
   terraform apply -var-file="secrets.tfvars"
   ```
4. **Apply K8s Manifests:**
   ```bash
   kubectl apply -f ../../k8s/
   ```

## 🔐 Security
- Secrets are managed via `tfvars` files (excluded from git).
- IAM roles are used for service-to-service authentication (EKS to S3/SQS).
