import os
import sys


def generate_big_file(filename, size_gb, chunk_size=1024 * 1024):
    target_size = int(size_gb * 1024 ** 3)
    current_size = 0

    with open(filename, 'wb') as f:
        while current_size < target_size:
            remaining = target_size - current_size
            bytes_to_generate = min(chunk_size, remaining)
            random_chunk = os.urandom(bytes_to_generate)
            f.write(random_chunk)
            current_size += len(random_chunk)
            print(f"Written {current_size} bytes...", end='\r')
    print()


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <output> <size_gb>")
        sys.exit(1)

    generate_big_file(sys.argv[1], float(sys.argv[2]))
    print("\nFile created successfully!")
