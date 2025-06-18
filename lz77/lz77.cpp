#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <deque>
#include <string>
#include <iomanip>
#include <cstdint>

using namespace std;

struct Token {
    uint16_t offset;
    uint16_t length;
    char nextChar;
};

const size_t WINDOW_SIZE = 4096;
const size_t LOOKAHEAD_SIZE = 18;
const size_t BUFFER_SIZE = 1 << 20; // 1MB

void compressFile(const string &inputPath, const string &outputPath) {
    ifstream input(inputPath, ios::binary);
    ofstream output(outputPath, ios::binary);

    if (!input || !output) {
        cerr << "Error opening files!" << endl;
        return;
    }

    deque<char> window;
    vector<char> buffer(BUFFER_SIZE);

    while (!input.eof()) {
        input.read(buffer.data(), BUFFER_SIZE);
        size_t bytesRead = input.gcount();
        size_t pos = 0;

        while (pos < bytesRead) {
            size_t maxLength = 0;
            size_t bestOffset = 0;

            for (size_t i = 1; i <= window.size(); ++i) {
                size_t length = 0;
                while (length < LOOKAHEAD_SIZE &&
                       pos + length < bytesRead &&
                       window[window.size() - i + length] == buffer[pos + length]) {
                    ++length;
                }

                if (length > maxLength) {
                    maxLength = length;
                    bestOffset = i;
                }
            }

            Token token;
            if (maxLength >= 3) {
                token.offset = bestOffset;
                token.length = maxLength;
                token.nextChar = (pos + maxLength < bytesRead) ? buffer[pos + maxLength] : '\0';
                pos += maxLength + 1;
            } else {
                token.offset = 0;
                token.length = 0;
                token.nextChar = buffer[pos++];
            }

            output.write(reinterpret_cast<char*>(&token), sizeof(Token));

            // Add bytes to window
            size_t added = token.length + 1;
            for (size_t i = 0; i < added && pos - added + i < bytesRead; ++i) {
                window.push_back(buffer[pos - added + i]);
                if (window.size() > WINDOW_SIZE)
                    window.pop_front();
            }
        }
    }

    cout << "Compression complete.\n";
}

void decompressFile(const string &inputPath, const string &outputPath) {
    ifstream input(inputPath, ios::binary);
    ofstream output(outputPath, ios::binary);

    if (!input || !output) {
        cerr << "Error opening files!" << endl;
        return;
    }

    deque<char> window;
    Token token;

    while (input.read(reinterpret_cast<char*>(&token), sizeof(Token))) {
        for (int i = 0; i < token.length; ++i) {
            char c = window[window.size() - token.offset + i];
            output.put(c);
            window.push_back(c);
            if (window.size() > WINDOW_SIZE)
                window.pop_front();
        }

        if (token.nextChar != '\0') {
            output.put(token.nextChar);
            window.push_back(token.nextChar);
            if (window.size() > WINDOW_SIZE)
                window.pop_front();
        }
    }

    cout << "Decompression complete.\n";
}

uint64_t getFileSize(const string& filePath) {
    ifstream file(filePath, ios::binary | ios::ate);
    if (!file) return UINT64_MAX;
    return static_cast<uint64_t>(file.tellg());
}

void printStats(const string& inputPath, const string& outputPath) {
    uint64_t original = getFileSize(inputPath);
    uint64_t compressed = getFileSize(outputPath);

    if (original == UINT64_MAX || compressed == UINT64_MAX) {
        cerr << "Error getting file size." << endl;
        return;
    }

    double ratio = (original > 0)
        ? (1.0 - static_cast<double>(compressed) / original) * 100.0
        : 0.0;

    cout << fixed << setprecision(2);
    cout << "Original size: " << original << " bytes\n";
    cout << "Compressed size: " << compressed << " bytes\n";
    cout << "Compression ratio: " << ratio << "%\n";
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: lz77 <compress|decompress> <input> <output>\n";
        return 1;
    }

    string mode = argv[1];
    string input = argv[2];
    string output = argv[3];

    if (mode == "compress") {
        compressFile(input, output);
        printStats(input, output);
    } else if (mode == "decompress") {
        decompressFile(input, output);
    } else {
        cerr << "Invalid mode! Use 'compress' or 'decompress'\n";
        return 1;
    }

    return 0;
}
