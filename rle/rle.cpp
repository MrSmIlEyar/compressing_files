#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstdint>

using namespace std;

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
