import os
import json
import boto3
import time
from sqlalchemy import create_engine, text
from faster_whisper import WhisperModel
from transformers import pipeline

# AWS and Database configuration
AWS_REGION = os.environ.get("AWS_REGION", "ap-south-1")
S3_BUCKET_NAME = os.environ.get("S3_BUCKET_NAME")
SQS_QUEUE_URL = os.environ.get("SQS_QUEUE_URL")
POSTGRES_URL = os.environ.get("POSTGRES_URL")

# Initialize AWS clients
s3_client = boto3.client('s3', region_name=AWS_REGION)
sqs_client = boto3.client('sqs', region_name=AWS_REGION)

# Initialize Database Engine
engine = create_engine(POSTGRES_URL) if POSTGRES_URL else None

# Initialize Models (Local)
print("Loading Whisper Model (tiny.en)...")
whisper_model = WhisperModel("tiny.en", device="cpu", compute_type="int8")

print("Loading Intent Classifier Model...")
intent_classifier = pipeline("zero-shot-classification", model="facebook/bart-large-mnli")


def process_audio(device_id, s3_key, original_timestamp):
    local_file = "/tmp/audio.wav"
    
    # 1. Download file
    print(f"Downloading {s3_key} from {S3_BUCKET_NAME}")
    s3_client.download_file(S3_BUCKET_NAME, s3_key, local_file)
    
    # 2. STT Inference
    print("Running STT inference...")
    segments, info = whisper_model.transcribe(local_file, beam_size=1)
    transcription = " ".join([segment.text for segment in segments]).strip()
    print(f"Transcription: {transcription}")
    
    # 3. Intent Inference
    print("Running Intent inference...")
    candidate_labels = ["question", "command", "statement", "greeting", "urgent"]
    result = intent_classifier(transcription, candidate_labels)
    tags = ",".join(result['labels'][:2]) # Get top 2 tags
    print(f"Extracted tags: {tags}")
    
    # 4. Save to Database
    if engine:
        with engine.connect() as conn:
            # Upsert logic to ensure device is registered (as fallback, but website handle signup)
            user_res = conn.execute(text("SELECT id FROM users WHERE device_id = :device_id"), {"device_id": device_id}).fetchone()
            
            if user_res:
                user_id = user_res[0]
                conn.execute(
                    text("INSERT INTO records (user_id, timestamp, transcription, tags) VALUES (:uid, :ts, :tr, :tg)"),
                    {"uid": user_id, "ts": original_timestamp, "tr": transcription, "tg": tags}
                )
                conn.commit()
                print(f"Saved to Database for user {user_id}")
            else:
                print(f"WARNING: No user found for device_id {device_id}. Data dropped.")
    
    # 5. Cleanup
    os.remove(local_file)
    s3_client.delete_object(Bucket=S3_BUCKET_NAME, Key=s3_key)
    print(f"Deleted S3 object {s3_key}")

def poll_sqs():
    print(f"Polling SQS {SQS_QUEUE_URL}...")
    while True:
        try:
            response = sqs_client.receive_message(
                QueueUrl=SQS_QUEUE_URL,
                MaxNumberOfMessages=1,
                WaitTimeSeconds=20
            )

            if "Messages" in response:
                for msg in response["Messages"]:
                    body = json.loads(msg["Body"])
                    receipt_handle = msg["ReceiptHandle"]
                    
                    device_id = body["device_id"]
                    s3_key = body["s3_key"]
                    timestamp = body["timestamp"]
                    
                    print(f"Processing Task: {device_id} -> {s3_key}")
                    try:
                        process_audio(device_id, s3_key, timestamp)
                        # Delete Message on success
                        sqs_client.delete_message(
                            QueueUrl=SQS_QUEUE_URL,
                            ReceiptHandle=receipt_handle
                        )
                        print("Task completed and message deleted from SQS.")
                    except Exception as e:
                        print(f"Failed to process task: {e}")
            else:
                pass # No messages, long polling timeout reached

        except Exception as e:
            print(f"SQS Polling Error: {e}")
            time.sleep(5)

if __name__ == "__main__":
    time.sleep(2) # Give dependencies time to initialize
    poll_sqs()
