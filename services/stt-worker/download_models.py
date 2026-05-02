import os
import sys
import subprocess
from faster_whisper.utils import download_model

def download_whisper():
    print("1. Downloading Whisper Model (tiny.en)...")
    download_model("tiny.en")

def download_bart():
    print("2. Downloading Intent Classifier Model (facebook/bart-large-mnli)...")
    # Using the CLI directly is often more memory-efficient than the Python API
    # We download to a specific local directory to simplify loading and avoid cache overhead
    model_dir = "/app/models/bart"
    os.makedirs(model_dir, exist_ok=True)
    
    cmd = [
        "huggingface-cli", "download", "facebook/bart-large-mnli",
        "--local-dir", model_dir,
        "--local-dir-use-symlinks", "False",
        "--exclude", "*.h5", "*.msgpack", "*.ot", "*.tf_*.bin"
    ]
    
    subprocess.run(cmd, check=True)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        target = sys.argv[1]
        if target == "whisper":
            download_whisper()
        elif target == "bart":
            download_bart()
    else:
        download_whisper()
        download_bart()

    print("Download step completed!")
