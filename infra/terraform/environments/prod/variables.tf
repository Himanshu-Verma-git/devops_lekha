variable "aws_region" {
  description = "AWS Region to deploy to"
  type        = string
  default     = "ap-south-1"
}

variable "cluster_name" {
  description = "EKS Cluster name"
  type        = string
  default     = "lekha-cluster"
}

variable "db_password" {
  description = "PostgreSQL password"
  type        = string
  sensitive   = true
  default     = "change_me_in_prod123"
}
