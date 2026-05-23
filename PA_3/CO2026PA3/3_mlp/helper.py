from pathlib import Path
import struct
import sys

NUM_TEST = 300
OUTPUT_DIM = 10
OUTPUT_SIZE = NUM_TEST * OUTPUT_DIM


def load_f32(path: str, count: int):
    data = Path(path).read_bytes()
    expected = count * 4
    if len(data) != expected:
        raise ValueError(f"{path}: expected {expected} bytes, got {len(data)} bytes")
    return struct.unpack(f"<{count}f", data)


def main():
    if len(sys.argv) < 2:
        print("Usage: python helper.py <binary_output_file>")
        return
    
    path = sys.argv[1]
    nums = load_f32(path, OUTPUT_SIZE)
    for i in range(0, NUM_TEST):
        for j in range(0, OUTPUT_DIM): 
            print(f"{nums[i*OUTPUT_DIM+j]:.3f}", end=" ")
        print()

if __name__ == "__main__":
    main()