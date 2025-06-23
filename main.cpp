#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>
#include <cstdint>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

// Function declarations
void encodeFile(const string& inputFile, const string& outputFile);
void decodeFile(const string& inputFile, const string& outputFile);
void compressFileLZ77(const string& inputPath, const string& outputPath);
void decompressFileLZ77(const string& inputPath, const string& outputPath);
void compressFileRLE(const string& inputPath, const string& outputPath);
void decompressFileRLE(const string& inputPath, const string& outputPath);

// Function to get file size
uint64_t getFileSize(const string& filePath) {
    ifstream file(filePath, ios::binary | ios::ate);
    if (!file) return 0;
    return static_cast<uint64_t>(file.tellg());
}

// Structure to store compression results
struct CompressionResult {
    string algorithm;
    string compressedPath;
    string decompressedPath;
    uint64_t originalSize;
    uint64_t compressedSize;
    uint64_t decompressedSize;
    double compressionTime;
    double decompressionTime;
    double ratio;
    bool integrity;
};

// Function to compare two files
bool compareFiles(const string& file1, const string& file2) {
    ifstream f1(file1, ios::binary);
    ifstream f2(file2, ios::binary);
    
    if (!f1 || !f2) return false;
    
    // Compare file sizes first
    f1.seekg(0, ios::end);
    f2.seekg(0, ios::end);
    if (f1.tellg() != f2.tellg()) return false;
    
    // Compare content
    f1.seekg(0);
    f2.seekg(0);
    return equal(istreambuf_iterator<char>(f1),
                 istreambuf_iterator<char>(),
                 istreambuf_iterator<char>(f2));
}

// Function to display ASCII bar chart
void printBarChart(double value, double maxValue, int width = 50) {
    int barWidth = static_cast<int>((value / maxValue) * width);
    cout << "│";
    for (int i = 0; i < barWidth; i++) cout << "█";
    for (int i = barWidth; i < width; i++) cout << " ";
    cout << "│ " << fixed << setprecision(2) << value << "%\n";
}

// Function to display detailed statistics
void displayStatistics(const vector<CompressionResult>& results) {
    if (results.empty()) return;
    
    // Find max values for scaling
    double maxRatio = 0;
    double maxTime = 0;
    for (const auto& res : results) {
        if (res.ratio > maxRatio) maxRatio = res.ratio;
        if (res.compressionTime > maxTime) maxTime = res.compressionTime;
        if (res.decompressionTime > maxTime) maxTime = res.decompressionTime;
    }
    maxRatio = max(maxRatio, 100.0);
    maxTime = max(maxTime, 1.0); // Ensure we don't divide by zero
    
    cout << "\n\n";
    cout << "╔══════════════════════════════════════════════════════════════════════════╗\n";
    cout << "║                      COMPRESSION ALGORITHM COMPARISON                   ║\n";
    cout << "╠═══════════════╦════════════╦════════════╦════════════╦════════╦═════════╣\n";
    cout << "║ Algorithm     ║ Compressed ║ Ratio (%)  ║ Comp Time  ║ Dec Time║ Integrity║\n";
    cout << "╠═══════════════╬════════════╬════════════╬════════════╬════════╬═════════╣\n";
    
    for (const auto& res : results) {
        cout << "║ " << left << setw(13) << res.algorithm << " ║ "
             << right << setw(10) << res.compressedSize << " ║ "
             << setw(10) << fixed << setprecision(2) << res.ratio << " ║ "
             << setw(10) << fixed << setprecision(3) << res.compressionTime << " ║ "
             << setw(8) << fixed << setprecision(3) << res.decompressionTime << " ║ "
             << setw(7) << (res.integrity ? "✓" : "✗") << " ║\n";
    }
    
    cout << "╚═══════════════╩════════════╩════════════╩════════════╩════════╩═════════╝\n";
    
    // Bar chart for compression ratio
    cout << "\n\nCOMPRESSION RATIO COMPARISON:\n";
    cout << "┌──────────────────────────────────────────────────┐\n";
    for (const auto& res : results) {
        cout << "│ " << left << setw(12) << res.algorithm << " ";
        printBarChart(res.ratio, maxRatio, 40);
    }
    cout << "└──────────────────────────────────────────────────┘\n";
    
    // Bar chart for compression time
    cout << "\n\nCOMPRESSION TIME (ms):\n";
    cout << "┌──────────────────────────────────────────────────┐\n";
    for (const auto& res : results) {
        cout << "│ " << left << setw(12) << res.algorithm << " ";
        printBarChart(res.compressionTime, maxTime, 40);
    }
    cout << "└──────────────────────────────────────────────────┘\n";
    
    // Bar chart for decompression time
    cout << "\n\nDECOMPRESSION TIME (ms):\n";
    cout << "┌──────────────────────────────────────────────────┐\n";
    for (const auto& res : results) {
        cout << "│ " << left << setw(12) << res.algorithm << " ";
        printBarChart(res.decompressionTime, maxTime, 40);
    }
    cout << "└──────────────────────────────────────────────────┘\n";
    
    // Detailed analysis
    cout << "\n\nDETAILED ANALYSIS:\n";
    cout << "┌───────────────────────────────────────────────────────────┐\n";
    cout << "│ ORIGINAL FILE SIZE: " << right << setw(10) << results[0].originalSize << " bytes\n";
    
    // Find best algorithm
    auto bestRatio = max_element(results.begin(), results.end(), 
        [](const CompressionResult& a, const CompressionResult& b) {
            return a.ratio < b.ratio;
        });
    
    auto bestCompTime = min_element(results.begin(), results.end(), 
        [](const CompressionResult& a, const CompressionResult& b) {
            return a.compressionTime < b.compressionTime;
        });
    
    auto bestDecompTime = min_element(results.begin(), results.end(), 
        [](const CompressionResult& a, const CompressionResult& b) {
            return a.decompressionTime < b.decompressionTime;
        });
    
    cout << "│ BEST RATIO: " << bestRatio->algorithm << " (" << bestRatio->ratio << "%)\n";
    cout << "│ FASTEST COMPRESSION: " << bestCompTime->algorithm 
         << " (" << bestCompTime->compressionTime << " ms)\n";
    cout << "│ FASTEST DECOMPRESSION: " << bestDecompTime->algorithm 
         << " (" << bestDecompTime->decompressionTime << " ms)\n";
    
    // Calculate space savings
    cout << "│ SPACE SAVINGS:\n";
    for (const auto& res : results) {
        double savings = (1.0 - static_cast<double>(res.compressedSize)/res.originalSize) * 100.0;
        cout << "│   " << left << setw(12) << res.algorithm << ": " 
             << fixed << setprecision(2) << savings << "%\n";
    }
    
    // Compression efficiency (bytes/ms)
    cout << "│ COMPRESSION EFFICIENCY (bytes/ms):\n";
    for (const auto& res : results) {
        double efficiency = res.originalSize / res.compressionTime;
        cout << "│   " << left << setw(12) << res.algorithm << ": " 
             << fixed << setprecision(2) << efficiency << "\n";
    }
    
    // Integrity check summary
    cout << "│ INTEGRITY CHECK: ";
    bool allGood = all_of(results.begin(), results.end(), 
        [](const CompressionResult& r) { return r.integrity; });
    
    if (allGood) {
        cout << "ALL ALGORITHMS PASSED ✓\n";
    } else {
        cout << "SOME ALGORITHMS FAILED:\n";
        for (const auto& res : results) {
            if (!res.integrity) {
                cout << "│   " << res.algorithm << " - decompressed file differs from original\n";
            }
        }
    }
    
    cout << "└───────────────────────────────────────────────────────────┘\n";
    
    // Recommendation
    cout << "\n\nRECOMMENDATION:\n";
    cout << "┌───────────────────────────────────────────────────────────┐\n";
    if (bestRatio->ratio > 30) {
        cout << "│ For significant space savings (" << bestRatio->ratio << "%), use " 
             << bestRatio->algorithm << " compression\n";
    } else if (results[0].originalSize < 1024) {
        cout << "│ For small files, compression may not be efficient. Best ratio: " 
             << bestRatio->ratio << "% with " << bestRatio->algorithm << "\n";
    } else {
        cout << "│ For balanced performance, consider " << bestCompTime->algorithm 
             << " (compression: " << bestCompTime->compressionTime << " ms, ratio: "
             << bestCompTime->ratio << "%)\n";
    }
    cout << "└───────────────────────────────────────────────────────────┘\n";
}

// Function to test all compression algorithms
void testAllAlgorithms(const string& inputFile) {
    vector<CompressionResult> results;
    vector<pair<string, string>> algorithms = {
        {"Huffman", ".huff"},
        {"LZ77", ".lz77"},
        {"RLE", ".rle"}
    };

    for (const auto& algo : algorithms) {
        CompressionResult result;
        result.algorithm = algo.first;
        result.originalSize = getFileSize(inputFile);
        
        // Generate filenames
        string compressedFile = inputFile + algo.second;
        string decompressedFile = inputFile + ".decompressed" + algo.second;
        
        // Compression
        auto startComp = chrono::high_resolution_clock::now();
        
        if (algo.first == "Huffman") {
            encodeFile(inputFile, compressedFile);
        } else if (algo.first == "LZ77") {
            compressFileLZ77(inputFile, compressedFile);
        } else if (algo.first == "RLE") {
            compressFileRLE(inputFile, compressedFile);
        }
        
        auto endComp = chrono::high_resolution_clock::now();
        result.compressionTime = chrono::duration<double, milli>(endComp - startComp).count();
        result.compressedSize = getFileSize(compressedFile);
        result.ratio = (1.0 - static_cast<double>(result.compressedSize)/result.originalSize) * 100.0;
        
        // Decompression
        auto startDecomp = chrono::high_resolution_clock::now();
        
        if (algo.first == "Huffman") {
            decodeFile(compressedFile, decompressedFile);
        } else if (algo.first == "LZ77") {
            decompressFileLZ77(compressedFile, decompressedFile);
        } else if (algo.first == "RLE") {
            decompressFileRLE(compressedFile, decompressedFile);
        }
        
        auto endDecomp = chrono::high_resolution_clock::now();
        result.decompressionTime = chrono::duration<double, milli>(endDecomp - startDecomp).count();
        result.decompressedSize = getFileSize(decompressedFile);
        
        // Integrity check
        result.integrity = compareFiles(inputFile, decompressedFile);
        
        // Store results
        results.push_back(result);
    }
    
    // Display comprehensive statistics
    displayStatistics(results);
    
    // Cleanup temporary files
    for (const auto& algo : algorithms) {
        string compressedFile = inputFile + algo.second;
        string decompressedFile = inputFile + ".decompressed" + algo.second;
        remove(compressedFile.c_str());
        remove(decompressedFile.c_str());
    }
}

int main() {
    int choice;
    string inputFile, outputFile;

    cout << "╔═══════════════════════════════════════════════════╗\n";
    cout << "║             ADVANCED FILE COMPRESSION SUITE       ║\n";
    cout << "╠═══════════════════════════════════════════════════╣\n";
    cout << "║ 1. Test all algorithms (comparison)               ║\n";
    cout << "║ 2. Use Huffman compression                        ║\n";
    cout << "║ 3. Use LZ77 compression                           ║\n";
    cout << "║ 4. Use RLE compression                            ║\n";
    cout << "║ 0. Exit                                           ║\n";
    cout << "╚═══════════════════════════════════════════════════╝\n";
    cout << "> ";
    cin >> choice;

    if (choice == 0) return 0;
    
    cout << "\nEnter input file path: ";
    cin >> inputFile;
    
    if (choice == 1) {
        testAllAlgorithms(inputFile);
        return 0;
    }
    
    cout << "Enter output file path: ";
    cin >> outputFile;

    try {
        if (choice == 2) { 
            encodeFile(inputFile, outputFile);
        } 
        else if (choice == 3) { 
            compressFileLZ77(inputFile, outputFile);
        } 
        else if (choice == 4) { 
            compressFileRLE(inputFile, outputFile);
        }
        
        uint64_t originalSize = getFileSize(inputFile);
        uint64_t compressedSize = getFileSize(outputFile);
        
        cout << "\nCompression complete!\n";
        cout << "Original size: " << originalSize << " bytes\n";
        cout << "Compressed size: " << compressedSize << " bytes\n";
        
        if (originalSize > 0) {
            double ratio = (1.0 - static_cast<double>(compressedSize)/originalSize) * 100.0;
            cout << "Compression ratio: " << fixed << setprecision(2) << ratio << "%\n";
        }
    } 
    catch (const exception& e) {
        cerr << "\nError: " << e.what() << endl;
        return 1;
    }

    return 0;
}