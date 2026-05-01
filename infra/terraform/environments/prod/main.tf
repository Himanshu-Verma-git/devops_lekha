# 1. VPC Configuration
module "vpc" {
  source  = "terraform-aws-modules/vpc/aws"
  version = "~> 5.0"

  name = "lekha-vpc"
  cidr = "10.0.0.0/16"

  azs             = ["${var.aws_region}a", "${var.aws_region}b"]
  private_subnets = ["10.0.1.0/24", "10.0.2.0/24"]
  public_subnets  = ["10.0.101.0/24", "10.0.102.0/24"]

  enable_nat_gateway = true
  single_nat_gateway = true
}

# 2. EKS Cluster
module "eks" {
  source  = "terraform-aws-modules/eks/aws"
  version = "~> 20.0"

  cluster_name    = var.cluster_name
  cluster_version = "1.29"

  vpc_id     = module.vpc.vpc_id
  subnet_ids = module.vpc.private_subnets

  # REQUIRED for node group creation in v20+
  enable_cluster_creator_admin_permissions = true

  eks_managed_node_groups = {
    general = {
      desired_size = 1
      min_size     = 1
      max_size     = 3

      instance_types = ["t3.medium"]

      # CRITICAL FIX (EKS 1.29 compatibility)
      ami_type = "AL2023_x86_64_STANDARD"

      iam_role_additional_policies = {
        app_policy = aws_iam_policy.node_app_policy.arn
      }
    }
  }
}
# module "eks" {
#   source  = "terraform-aws-modules/eks/aws"
#   version = "~> 20.0"
# 
#   cluster_name    = var.cluster_name
#   cluster_version = "1.29"
# 
#   vpc_id     = module.vpc.vpc_id
#   subnet_ids = module.vpc.private_subnets
# 
#   eks_managed_node_groups = {
#     general = {
#       desired_size = 1
#       min_size     = 1
#       max_size     = 3
# 
#       instance_types = ["t3.medium"] # Small generic instance for EKS workers
#     }
#   }
# }
# 


# 3. S3 Bucket for Audio
resource "aws_s3_bucket" "lekha_audio_bucket" {
  bucket_prefix = "lekha-audio-artifacts-"
}

resource "aws_s3_bucket_public_access_block" "lekha_audio_block" {
  bucket                  = aws_s3_bucket.lekha_audio_bucket.id
  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

# 4. SQS Queue for processing audio tasks
resource "aws_sqs_queue" "lekha_audio_queue" {
  name                      = "lekha-audio-processing.fifo"
  fifo_queue                = true
  content_based_deduplication = true
  visibility_timeout_seconds = 300 # Allow up to 5 min for inference
}

# IAM Policy for EKS Nodes to access S3 and SQS
resource "aws_iam_policy" "node_app_policy" {
  name        = "lekha-node-app-policy"
  description = "Permissions for Lekha application on EKS nodes"
  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect = "Allow"
        Action = [
          "s3:PutObject",
          "s3:GetObject",
          "s3:DeleteObject"
        ]
        Resource = [
          aws_s3_bucket.lekha_audio_bucket.arn,
          "${aws_s3_bucket.lekha_audio_bucket.arn}/*"
        ]
      },
      {
        Effect = "Allow"
        Action = [
          "sqs:ReceiveMessage",
          "sqs:DeleteMessage",
          "sqs:SendMessage",
          "sqs:GetQueueUrl"
        ]
        Resource = aws_sqs_queue.lekha_audio_queue.arn
      }
    ]
  })
}

# 5. RDS PostgreSQL Database
resource "aws_db_subnet_group" "lekha_db_subnet" {
  name       = "lekha-db-subnet-group"
  subnet_ids = module.vpc.private_subnets
}

resource "aws_security_group" "rds_sg" {
  name        = "lekha_rds_sg"
  description = "Allow inside VPC communication for RDS"
  vpc_id      = module.vpc.vpc_id

  ingress {
    from_port   = 5432
    to_port     = 5432
    protocol    = "tcp"
    cidr_blocks = [module.vpc.vpc_cidr_block]
  }
}

resource "aws_db_instance" "lekha_db" {
  identifier           = "lekha-db"
  engine               = "postgres"
  engine_version       = "15"
  instance_class       = "db.t3.micro" # Cheapest option as requested
  allocated_storage    = 20
  db_name              = "lekhadb"
  username             = "lekhaadmin"
  password             = var.db_password
  db_subnet_group_name = aws_db_subnet_group.lekha_db_subnet.name
  vpc_security_group_ids = [aws_security_group.rds_sg.id]
  skip_final_snapshot  = true
  publicly_accessible  = false
}

# 6. EC2 Instance for Website
data "aws_ami" "ubuntu" {
  most_recent = true
  owners      = ["099720109477"] # Canonical

  filter {
    name   = "name"
    values = ["ubuntu/images/hvm-ssd/ubuntu-jammy-22.04-amd64-server-*"]
  }
}

resource "aws_security_group" "ec2_sg" {
  name        = "lekha_ec2_sg"
  description = "Allow HTTP/HTTPS to Website"
  vpc_id      = module.vpc.vpc_id

  ingress {
    from_port   = 80
    to_port     = 80
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }
  
  ingress {
    from_port   = 443
    to_port     = 443
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }
  
  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}

resource "aws_instance" "lekha_website" {
  ami           = data.aws_ami.ubuntu.id
  instance_type = "t3.micro"
  
  subnet_id                   = module.vpc.public_subnets[0]
  vpc_security_group_ids      = [aws_security_group.ec2_sg.id]
  associate_public_ip_address = true
  
  user_data = <<-EOF
              #!/bin/bash
              apt-get update
              apt-get install -y docker.io docker-compose
              systemctl enable docker
              systemctl start docker
              EOF

  tags = {
    Name = "lekha-website"
  }
}

# Outputs
output "s3_bucket_name" {
  value = aws_s3_bucket.lekha_audio_bucket.id
}
output "sqs_queue_url" {
  value = aws_sqs_queue.lekha_audio_queue.url
}
output "rds_endpoint" {
  value = aws_db_instance.lekha_db.endpoint
}
output "website_ip" {
  value = aws_instance.lekha_website.public_ip
}
