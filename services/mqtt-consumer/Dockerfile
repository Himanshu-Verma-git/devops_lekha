FROM python:3.11-slim

WORKDIR /app

# Install build deps
RUN apt-get update 
RUN apt-get install -y gcc
RUN apt-get install -y libffi-dev
RUN apt-get install -y python3-dev
RUN rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
RUN pip install --upgrade pip && \
    pip install --no-cache-dir -r requirements.txt

COPY main.py .

CMD ["python", "-u", "main.py"]