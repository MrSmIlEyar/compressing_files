// lz77.cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#include <cstdint>

using namespace std;

const size_t WINDOW_SIZE = 4096;    // Размер окна
const size_t MAX_MATCH_LEN = 18;    // Максимальная длина совпадения
const size_t MIN_MATCH_LEN = 3;     // Минимальная длина совпадения

struct Token {
    uint16_t offset;
    uint16_t length;
    uint8_t nextChar;
};

// Функция сжатия LZ77
void compressFileLZ77(const string& inputPath, const string& outputPath) {
    ifstream in(inputPath, ios::binary);
    if (!in) {
        cerr << "Error: Cannot open input file!" << endl;
        return;
    }
    
    // Определение размера файла
    in.seekg(0, ios::end);
    size_t fileSize = in.tellg();
    in.seekg(0, ios::beg);
    
    // Для очень маленьких файлов (менее 64 байт) - не сжимаем
    if (fileSize < 64) {
        ofstream out(outputPath, ios::binary);
        out.put('U'); // Маркер несжатых данных
        out << in.rdbuf();
        cout << "Small file stored without compression (" << fileSize << " bytes)" << endl;
        return;
    }
    
    ofstream out(outputPath, ios::binary);
    if (!out) {
        cerr << "Error: Cannot open output file!" << endl;
        return;
    }
    
    // Запись маркера сжатых данных
    out.put('C');
    
    // Буфер данных
    vector<uint8_t> data(fileSize);
    in.read(reinterpret_cast<char*>(data.data()), fileSize);
    in.close();
    
    // Скользящее окно и буфер предпросмотра
    deque<uint8_t> window;
    size_t pos = 0;
    
    while (pos < data.size()) {
        size_t bestOffset = 0;
        size_t bestLength = 0;
        
        // Поиск наилучшего совпадения в окне
        for (size_t i = 0; i < window.size(); i++) {
            size_t len = 0;
            while (len < MAX_MATCH_LEN && 
                   pos + len < data.size() && 
                   window[window.size() - i - 1 + len] == data[pos + len]) {
                len++;
            }
            
            if (len > bestLength && len >= MIN_MATCH_LEN) {
                bestLength = len;
                bestOffset = i;
            }
        }
        
        Token token;
        if (bestLength >= MIN_MATCH_LEN) {
            token.offset = static_cast<uint16_t>(bestOffset);
            token.length = static_cast<uint16_t>(bestLength);
            token.nextChar = (pos + bestLength < data.size()) ? data[pos + bestLength] : 0;
            
            // Добавление в окно
            size_t addCount = min(bestLength + 1, data.size() - pos);
            for (size_t i = 0; i < addCount; i++) {
                window.push_back(data[pos + i]);
                if (window.size() > WINDOW_SIZE) {
                    window.pop_front();
                }
            }
            pos += addCount;
        } else {
            token.offset = 0;
            token.length = 0;
            token.nextChar = data[pos];
            
            window.push_back(data[pos]);
            if (window.size() > WINDOW_SIZE) {
                window.pop_front();
            }
            pos++;
        }
        
        // Запись токена
        out.write(reinterpret_cast<const char*>(&token), sizeof(token));
    }
    
    cout << "File compressed successfully: " << fileSize << " -> " 
         << out.tellp() << " bytes" << endl;
}

// Функция распаковки LZ77
void decompressFileLZ77(const string& inputPath, const string& outputPath) {
    ifstream in(inputPath, ios::binary);
    if (!in) {
        cerr << "Error: Cannot open input file!" << endl;
        return;
    }
    
    // Проверка маркера
    char marker = in.get();
    if (marker == 'U') {
        // Несжатые данные
        ofstream out(outputPath, ios::binary);
        out << in.rdbuf();
        cout << "Small file extracted successfully" << endl;
        return;
    } else if (marker != 'C') {
        cerr << "Error: Invalid file format!" << endl;
        return;
    }
    
    ofstream out(outputPath, ios::binary);
    if (!out) {
        cerr << "Error: Cannot open output file!" << endl;
        return;
    }
    
    // Буфер данных
    vector<uint8_t> output;
    vector<uint8_t> window;
    
    Token token;
    while (in.read(reinterpret_cast<char*>(&token), sizeof(token))) {
        if (token.length > 0) {
            // Обработка совпадения
            size_t start = window.size() - token.offset;
            for (size_t i = 0; i < token.length; i++) {
                uint8_t byte = window[start + i];
                output.push_back(byte);
                window.push_back(byte);
                if (window.size() > WINDOW_SIZE) {
                    window.erase(window.begin());
                }
            }
        }
        
        // Добавление нового символа
        if (token.nextChar != 0) {
            output.push_back(token.nextChar);
            window.push_back(token.nextChar);
            if (window.size() > WINDOW_SIZE) {
                window.erase(window.begin());
            }
        }
    }
    
    // Запись распакованных данных
    out.write(reinterpret_cast<const char*>(output.data()), output.size());
    cout << "File decompressed successfully: " << output.size() << " bytes" << endl;
}