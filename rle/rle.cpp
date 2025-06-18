#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstdint>

using namespace std;

uint64_t getFileSize(const string& filePath) {
    ifstream file(filePath, ios::binary | ios::ate);
    if (!file) return UINT64_MAX;
    streampos pos = file.tellg();
    return static_cast<uint64_t>(pos);
}

void printFileStats(const string& inputPath, const string& outputPath) {
    uint64_t originalSize = getFileSize(inputPath);
    uint64_t compressedSize = getFileSize(outputPath);
    
    if (originalSize == UINT64_MAX || compressedSize == UINT64_MAX) {
        cerr << "Error getting file sizes!" << endl;
        return;
    }

    double ratio = (originalSize > 0) 
        ? (1.0 - static_cast<double>(compressedSize) / originalSize) * 100.0
        : 0.0;

    cout << "Original size: " << originalSize << " bytes\n"
         << "Compressed size: " << compressedSize << " bytes\n"
         << "Compression ratio: " << fixed << setprecision(2) 
         << ratio << "%\n";
}

void compressFileRLE(const string& inputPath, const string& outputPath) {
    ifstream input(inputPath, ios::binary);
    ofstream output(outputPath, ios::binary);

    if (!input || !output) {
        cerr << "Error opening files!" << endl;
        return;
    }

    const size_t BUFFER_SIZE = 1 << 20;
    vector<unsigned char> buffer(BUFFER_SIZE);
    input.read(reinterpret_cast<char*>(buffer.data()), BUFFER_SIZE);
    size_t bytesRead = input.gcount();

    size_t i = 0;
    while (i < bytesRead) {
        size_t runStart = i;
        size_t runLength = 1;

        while (i + runLength < bytesRead && buffer[i] == buffer[i + runLength] && runLength < 127)
            ++runLength;

        if (runLength >= 2) {
            output.put(static_cast<unsigned char>(0x80 + runLength));
            output.put(buffer[i]);
            i += runLength;
        } else {
            size_t rawStart = i;
            size_t rawLen = 0;
            while ((i + rawLen < bytesRead) &&
                   (rawLen < 128) &&
                   !(i + rawLen + 1 < bytesRead &&
                     buffer[i + rawLen] == buffer[i + rawLen + 1])) {
                ++rawLen;
            }

            output.put(static_cast<unsigned char>(rawLen - 1));
            output.write(reinterpret_cast<char*>(&buffer[rawStart]), rawLen);
            i += rawLen;
        }

        if (i >= bytesRead && input) {
            input.read(reinterpret_cast<char*>(buffer.data()), BUFFER_SIZE);
            bytesRead = input.gcount();
            i = 0;
        }
    }

    input.close();
    output.close();
}


void decompressFileRLE(const string& inputPath, const string& outputPath) {
    ifstream input(inputPath, ios::binary);
    ofstream output(outputPath, ios::binary);

    if (!input || !output) {
        cerr << "Error opening files!" << endl;
        return;
    }

    while (true) {
        int control = input.get();
        if (control == EOF) break;

        if (control >= 0x80) {
            int count = control - 0x80;
            int value = input.get();
            if (value == EOF) break;

            for (int i = 0; i < count; ++i)
                output.put(static_cast<unsigned char>(value));
        } else {
            int count = control + 1;
            vector<char> raw(count);
            input.read(raw.data(), count);
            output.write(raw.data(), input.gcount());
        }
    }

    input.close();
    output.close();
}


int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: rle.exe <compress|decompress> <input> <output>\n";
        return 1;
    }

    string mode = argv[1];
    string input = argv[2];
    string output = argv[3];

    uint64_t size = getFileSize(input);
    if (size == UINT64_MAX) {
        cerr << "Error reading input file!" << endl;
        return 1;
    }

    if (mode == "compress") {
        compressFileRLE(input, output);
        cout << "Compression complete!\n";
        printFileStats(input, output);
    } else if (mode == "decompress") {
        decompressFileRLE(input, output);
        cout << "Decompression complete!\n";
        uint64_t outSize = getFileSize(output);
        if (outSize != UINT64_MAX) {
            cout << "Decompressed size: " << outSize << " bytes\n";
        }
    } else {
        cerr << "Invalid mode!" << endl;
        return 1;
    }

    return 0;
}