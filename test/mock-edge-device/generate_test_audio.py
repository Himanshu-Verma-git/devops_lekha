import wave
import struct
import math
import os

# Generates a simple sine wave test audio file
def generate_wav(file_path, duration_seconds=3, frequency=440, sample_rate=16000):
    os.makedirs(os.path.dirname(file_path), exist_ok=True)
    
    with wave.open(file_path, 'wb') as wav_file:
        n_channels = 1
        sampwidth = 2
        n_frames = duration_seconds * sample_rate
        
        wav_file.setnchannels(n_channels)
        wav_file.setsampwidth(sampwidth)
        wav_file.setframerate(sample_rate)
        
        for i in range(n_frames):
            value = int(32767.0 * math.sin(2.0 * math.pi * frequency * i / sample_rate))
            data = struct.pack('<h', value)
            wav_file.writeframesraw(data)

if __name__ == "__main__":
    wav_dir = "my_wavs"
    print(f"Generating test audio file in {wav_dir}/test_audio.wav...")
    generate_wav(f"{wav_dir}/test_audio.wav")
    print("Done!")
