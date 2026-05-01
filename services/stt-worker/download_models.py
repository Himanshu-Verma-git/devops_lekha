from faster_whisper import WhisperModel
from transformers import pipeline
import os

print("Starting to pre-download models to cache...")
print(f"HF_HOME is set to: {os.environ.get('HF_HOME')}")

print("1. Downloading Whisper Model (tiny.en)...")
# Download the model and ensure it's saved locally.
model = WhisperModel("tiny.en", device="cpu", compute_type="int8")

print("2. Downloading Intent Classifier Model...")
intent_classifier = pipeline("zero-shot-classification", model="facebook/bart-large-mnli")

print("Models downloaded successfully!")
