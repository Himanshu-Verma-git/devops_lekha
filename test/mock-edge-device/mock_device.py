import os
import time
import paho.mqtt.client as mqtt

MQTT_BROKER_URL = os.environ.get("MQTT_BROKER_URL")
MQTT_PORT = int(os.environ.get("MQTT_PORT", 1883))
DEVICE_ID = os.environ.get("DEVICE_ID", "mock_device_001")
WAV_DIR = os.environ.get("WAV_DIR", "/app/wav_files")

def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT broker with result code {rc}")

def main():
    if not MQTT_BROKER_URL:
        print("ERROR: MQTT_BROKER_URL environment variable is not set. Please provide it.")
        return

    client = mqtt.Client()
    client.on_connect = on_connect

    print(f"Connecting to MQTT Broker at {MQTT_BROKER_URL}:{MQTT_PORT}")
    client.connect(MQTT_BROKER_URL, MQTT_PORT, 60)
    client.loop_start()

    topic = f"audio/{DEVICE_ID}"

    # Wait briefly to ensure connection
    time.sleep(2)

    if not os.path.exists(WAV_DIR):
        print(f"Warning: Directory {WAV_DIR} does not exist.")
        return

    wav_files = [f for f in os.listdir(WAV_DIR) if f.endswith('.wav')]
    
    if not wav_files:
        print(f"No .wav files found in {WAV_DIR}")
        return

    for file_name in wav_files:
        file_path = os.path.join(WAV_DIR, file_name)
        print(f"Publishing {file_name} to topic {topic}...")
        
        try:
            with open(file_path, "rb") as f:
                payload = f.read()
                
            info = client.publish(topic, payload, qos=1)
            info.wait_for_publish()
            print(f"Successfully published {file_name} ({len(payload)} bytes)")
            
        except Exception as e:
            print(f"Failed to publish {file_name}: {e}")
            
        time.sleep(5) # wait between files
        
    print("All files published.")
    client.loop_stop()
    client.disconnect()

if __name__ == "__main__":
    main()
