FROM python:3.10-slim

WORKDIR /app

# Install system dependencies for audio processing and build tools
RUN apt-get update && apt-get install -y ffmpeg build-essential 
RUN rm -rf /var/lib/apt/lists/*

# COPY requirements.txt .
# RUN pip install --no-cache-dir -r requirements.txt
COPY requirements.txt .
RUN pip install --upgrade pip
    # pip install --no-cache-dir -r requirements.txt

RUN pip install --no-cache-dir boto3==1.34.0
RUN pip install --no-cache-dir SQLAlchemy==2.0.23
RUN pip install --no-cache-dir psycopg2-binary==2.9.9
ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies (including PyAV requirements)
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        pkg-config \
        libavcodec-dev \
        libavformat-dev \
        libavdevice-dev \
        libavutil-dev \
        libavfilter-dev \
        libswscale-dev \
        libswresample-dev && \
    rm -rf /var/lib/apt/lists/*
RUN pip install --no-cache-dir "av==12.0.0" --only-binary=:all:
RUN pip install --no-cache-dir faster-whisper==0.10.0
RUN pip install --no-cache-dir transformers==4.35.2
RUN pip install --no-cache-dir torch==2.1.2

COPY main.py .

CMD ["python", "-u", "main.py"]
