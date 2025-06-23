#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

using namespace std;

// Узел дерева Хаффмана с shared_ptr для удобства управления памятью
struct Node {
    uint8_t ch;
    int freq;
    shared_ptr<Node> left;
    shared_ptr<Node> right;

    // Конструктор для листового узла
    Node(uint8_t c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
    // Конструктор для внутреннего узла
    Node(int f, shared_ptr<Node> l, shared_ptr<Node> r)
        : ch(0), freq(f), left(move(l)), right(move(r)) {}
};

// Компаратор для очереди с shared_ptr
struct Compare {
    bool operator()(const shared_ptr<Node>& a, const shared_ptr<Node>& b) {
        return a->freq > b->freq;
    }
};

// Построение дерева Хаффмана с использованием shared_ptr
shared_ptr<Node> buildTree(const unordered_map<uint8_t, int>& freqMap) {
    priority_queue<shared_ptr<Node>, vector<shared_ptr<Node>>, Compare> pq;

    // Создаём листовые узлы
    for (const auto& p : freqMap) {
        pq.push(make_shared<Node>(p.first, p.second));
    }

    // Строим дерево
    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();

        int sumFreq = left->freq + right->freq;
        pq.push(make_shared<Node>(sumFreq, left, right));
    }

    if (pq.empty()) return nullptr;
    return pq.top();
}

// Рекурсивная генерация кодов Хаффмана
void generateCodes(const Node* node, const string& code, unordered_map<uint8_t, string>& codes) {
    if (!node) return;

    if (!node->left && !node->right) {
        codes[node->ch] = code;
        return;
    }

    generateCodes(node->left.get(), code + "0", codes);
    generateCodes(node->right.get(), code + "1", codes);
}

// Класс для записи битов в поток
class BitWriter {
    ostream& out_;
    uint8_t buffer_ = 0;
    int bitPos_ = 0;

public:
    BitWriter(ostream& out) : out_(out) {}

    ~BitWriter() {
        flush();
    }

    void writeBit(bool bit) {
        if (bit) {
            buffer_ |= (1 << (7 - bitPos_));
        }
        ++bitPos_;

        if (bitPos_ == 8) {
            out_.put(buffer_);
            buffer_ = 0;
            bitPos_ = 0;
        }
    }

    void writeBits(const string& bits) {
        for (char c : bits) {
            writeBit(c == '1');
        }
    }

    void flush() {
        if (bitPos_ > 0) {
            out_.put(buffer_);
            buffer_ = 0;
            bitPos_ = 0;
        }
    }
};

// Класс для чтения битов из потока
class BitReader {
    istream& in_;
    uint8_t buffer_ = 0;
    int bitPos_ = 8;

public:
    BitReader(istream& in) : in_(in) {}

    bool readBit(bool& bit) {
        if (bitPos_ == 8) {
            int byte = in_.get();
            if (byte == EOF) return false;
            buffer_ = static_cast<uint8_t>(byte);
            bitPos_ = 0;
        }

        bit = (buffer_ >> (7 - bitPos_)) & 1;
        ++bitPos_;
        return true;
    }
};

// Функция сжатия файла
void encodeFile(const string& inputFile, const string& outputFile) {
    ifstream in(inputFile, ios::binary);
    if (!in) {
        cerr << "Cannot open input file\n";
        return;
    }

    vector<uint8_t> data((istreambuf_iterator<char>(in)), {});
    in.close();

    if (data.size() < 32) {
        ofstream out(outputFile, ios::binary);
        out.put('U'); // Маркер несжатого файла
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        return;
    }

    unordered_map<uint8_t, int> freqMap;
    for (uint8_t c : data) freqMap[c]++;

    auto root = buildTree(freqMap);
    unordered_map<uint8_t, string> codes;
    generateCodes(root.get(), "", codes);

    ofstream out(outputFile, ios::binary);
    out.put('C'); // Маркер сжатого файла
    uint32_t dataSize = data.size();
    out.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    uint16_t uniqueCount = freqMap.size();
    out.write(reinterpret_cast<const char*>(&uniqueCount), sizeof(uniqueCount));

    for (const auto& p : freqMap) {
        out.put(p.first);
        uint32_t freq = p.second;
        out.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
    }

    BitWriter writer(out);
    for (uint8_t c : data) {
        writer.writeBits(codes[c]);
    }
}

// Функция распаковки файла
void decodeFile(const string& inputFile, const string& outputFile) {
    ifstream in(inputFile, ios::binary);
    if (!in) {
        cerr << "Cannot open input file\n";
        return;
    }

    char marker;
    in.get(marker);

    if (marker == 'U') {
        // Несжатый файл — просто копируем
        ofstream out(outputFile);
        char c;
        while (in.get(c)) out.put(c);
        return;
    }

    if (marker != 'C') {
        cerr << "Invalid file format\n";
        return;
    }

    uint32_t dataSize;
    if (!in.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize))) {
        cerr << "Failed to read data size\n";
        return;
    }

    uint16_t uniqueCount;
    if (!in.read(reinterpret_cast<char*>(&uniqueCount), sizeof(uniqueCount))) {
        cerr << "Failed to read unique count\n";
        return;
    }

    unordered_map<uint8_t, int> freqMap;
    for (int i = 0; i < uniqueCount; ++i) {
        uint8_t c = in.get();
        if (in.eof()) {
            cerr << "Unexpected end of file\n";
            return;
        }
        uint32_t freq;
        if (!in.read(reinterpret_cast<char*>(&freq), sizeof(freq))) {
            cerr << "Failed to read frequency\n";
            return;
        }
        freqMap[c] = freq;
    }

    auto root = buildTree(freqMap);
    if (!root) {
        cerr << "Failed to build Huffman tree\n";
        return;
    }

    // Открываем файл в текстовом режиме для записи символов
    ofstream out(outputFile);

    // Обработка случая дерева из одного узла
    if (!root->left && !root->right) {
        for (uint32_t i = 0; i < dataSize; ++i) {
            out.put(static_cast<char>(root->ch));
        }
        return;
    }

    BitReader reader(in);
    const Node* node = root.get();
    uint32_t written = 0;
    bool bit;

    while (written < dataSize) {
        if (!reader.readBit(bit)) {
            cerr << "Unexpected end of file\n";
            break;
        }

        node = bit ? node->right.get() : node->left.get();
        if (!node) {
            cerr << "Invalid bit sequence\n";
            break;
        }

        if (!node->left && !node->right) {
            out.put(static_cast<char>(node->ch));
            node = root.get();
            ++written;
        }
    }

    if (written != dataSize) {
        cerr << "Size mismatch: expected " << dataSize << ", decoded " << written << "\n";
    }
}
