FROM python:3.9-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY mock_device.py .

# Mount point for wav files
VOLUME /app/wav_files

CMD ["python", "mock_device.py"]
