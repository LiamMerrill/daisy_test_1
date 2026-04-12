import librosa
import soundfile as sf
import sys

if len(sys.argv) != 3:
    print("Usage: python resample_wav.py input.wav output.wav")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

print(f"Loading {input_file}...")
# Load and resample to 48000 Hz, convert to mono
y, sr = librosa.load(input_file, sr=48000, mono=True)

print(f"Duration: {len(y)/48000:.2f} seconds")
print(f"Saving to {output_file} at 48000 Hz, 16-bit...")

# Save as 16-bit WAV at 48000 Hz
sf.write(output_file, y, 48000, subtype='PCM_16')

print("Done!")