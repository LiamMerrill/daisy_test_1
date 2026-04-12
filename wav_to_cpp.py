import wave
import sys
import struct

def wav_to_cpp(wav_file, output_file, array_name):
    # Read WAV file
    with wave.open(wav_file, 'rb') as wav:
        frames = wav.readframes(wav.getnframes())
        sample_rate = wav.getframerate()
        num_channels = wav.getnchannels()
        sample_width = wav.getsampwidth()
        
        print(f"Reading {wav_file}:")
        print(f"  Sample rate: {sample_rate} Hz")
        print(f"  Channels: {num_channels}")
        print(f"  Bit depth: {sample_width * 8}")
        print(f"  Samples: {wav.getnframes()}")
        
        # Convert to 16-bit integers
        if sample_width == 2:  # 16-bit
            samples = struct.unpack(f'{len(frames)//2}h', frames)
        elif sample_width == 3:  # 24-bit
            # Convert 24-bit to 16-bit
            samples = []
            for i in range(0, len(frames), 3):
                val = int.from_bytes(frames[i:i+3], byteorder='little', signed=True)
                samples.append(val >> 8)  # Shift to 16-bit
        else:
            print(f"Error: Unsupported bit depth {sample_width * 8}")
            return
        
        # If stereo, take only left channel
        if num_channels == 2:
            samples = samples[::2]
        
        # Normalize to float [-1.0, 1.0]
        samples = [s / 32768.0 for s in samples]
    
    # Write C++ header
    with open(output_file, 'w') as f:
        f.write(f"// Generated from {wav_file}\n")
        f.write(f"// Sample rate: {sample_rate} Hz\n")
        f.write(f"// Length: {len(samples)} samples ({len(samples)/sample_rate:.2f} seconds)\n\n")
        f.write(f"const int {array_name}_length = {len(samples)};\n")
        f.write(f"const int {array_name}_sample_rate = {sample_rate};\n")
        f.write(f"const float {array_name}[{len(samples)}] = {{\n")
        
        # Write samples (10 per line for readability)
        for i in range(0, len(samples), 10):
            chunk = samples[i:i+10]
            line = ", ".join(f"{s:.6f}f" for s in chunk)
            f.write(f"    {line}")
            if i + 10 < len(samples):
                f.write(",\n")
            else:
                f.write("\n")
        
        f.write("};\n")
    
    print(f"\nCreated {output_file}")
    print(f"  Array name: {array_name}")
    print(f"  Length: {len(samples)} samples")
    print(f"  Duration: {len(samples)/sample_rate:.2f} seconds\n")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python wav_to_cpp.py input.wav output.h array_name")
        print("Example: python wav_to_cpp.py clap.wav clap_sample.h clap_sample")
        sys.exit(1)
    
    wav_to_cpp(sys.argv[1], sys.argv[2], sys.argv[3])