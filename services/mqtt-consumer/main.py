import os
import paho.mqtt.client as mqtt
import boto3
import json
import uuid
import time
from datetime import datetime

MQTT_BROKER_URL = os.environ.get("MQTT_BROKER_URL", "localhost")
MQTT_PORT = int(os.environ.get("MQTT_PORT", 1883))
S3_BUCKET_NAME = os.environ.get("S3_BUCKET_NAME")
SQS_QUEUE_URL = os.environ.get("SQS_QUEUE_URL")
AWS_REGION = os.environ.get("AWS_REGION", "ap-south-1")

s3_client = boto3.client('s3', region_name=AWS_REGION)
sqs_client = boto3.client('sqs', region_name=AWS_REGION)

def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT broker with result code {rc}")
    client.subscribe("audio/#")

def on_message(client, userdata, msg):
    try:
        topic = msg.topic
        device_id = topic.split("/")[-1]
        payload = msg.payload
        
        print(f"Received message on topic {topic}, size: {len(payload)} bytes")
        
        # Save payload to S3
        file_key = f"{device_id}/{uuid.uuid4()}.wav"
        
        s3_client.put_object(
            Bucket=S3_BUCKET_NAME,
            Key=file_key,
            Body=payload,
            ContentType="audio/wav"
        )
        print(f"Uploaded audio to S3: s3://{S3_BUCKET_NAME}/{file_key}")
        
        # Send message to SQS (FIFO Queue)
        message_body = {
            "device_id": device_id,
            "s3_key": file_key,
            "timestamp": datetime.utcnow().isoformat()
        }
        
        sqs_client.send_message(
            QueueUrl=SQS_QUEUE_URL,
            MessageBody=json.dumps(message_body),
            MessageGroupId=device_id, # Process sequentially per device
            MessageDeduplicationId=str(uuid.uuid4())
        )
        print(f"Sent task to SQS for {file_key}")
        
    except Exception as e:
        print(f"Error processing message: {e}")

if __name__ == "__main__":
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"Connecting to MQTT Broker at {MQTT_BROKER_URL}:{MQTT_PORT}")
    client.connect(MQTT_BROKER_URL, MQTT_PORT, 60)
    
    # Check AWS configuration
    if not S3_BUCKET_NAME or not SQS_QUEUE_URL:
        print("WARNING: S3_BUCKET_NAME or SQS_QUEUE_URL not set!")
        
    print("MQTT Consumer started.")
    client.loop_forever()
