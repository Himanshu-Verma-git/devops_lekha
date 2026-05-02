FROM python:3.10-slim

# Prevent interactive prompts during apt-get
ENV DEBIAN_FRONTEND=noninteractive

# Set a consistent cache directory for HuggingFace and Whisper models
# This ensures models downloaded during build are persisted in a known path
ENV HF_HOME=/app/models

WORKDIR /app

# 1. Install system dependencies in a single layer
# Includes audio processing (ffmpeg), database clients (libpq-dev), and PyAV requirements
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        ffmpeg \
        libpq-dev \
        pkg-config \
        libavcodec-dev \
        libavformat-dev \
        libavdevice-dev \
        libavutil-dev \
        libavfilter-dev \
        libswscale-dev \
        libswresample-dev && \
    rm -rf /var/lib/apt/lists/*

# 2. Install Python dependencies via requirements.txt
COPY requirements.txt .
RUN pip install --upgrade pip && \
    pip install --no-cache-dir -r requirements.txt

# 3. Pre-download models to bake them into the image
# We split these into separate layers and scripts to minimize memory pressure (exit code 137)
COPY download_models.py .
RUN python download_models.py whisper
RUN python download_models.py bart

# 4. Copy application source
COPY main.py .

# 5. Set default command
CMD ["python", "-u", "main.py"]
