#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <unordered_map>
#include <vector>
#include <bitset>

using namespace std;

struct Node {
    char ch;
    int freq;
    Node *left, *right;
    Node(char c, int f, Node* l = nullptr, Node* r = nullptr) : ch(c), freq(f), left(l), right(r) {}
};

// Сравнение для приоритетной очереди
struct comp {
    bool operator()(Node* l, Node* r) {
        return l->freq > r->freq;
    }
};

// Вывод дерева с отступами (рекурсивно)
void printTree(Node* root, string prefix = "") {
    if (!root) return;
    if (!root->left && !root->right) {
        cout << prefix << "'" << root->ch << "' (" << root->freq << ")\n";
    } else {
        cout << prefix << "* (" << root->freq << ")\n";
        printTree(root->left, prefix + " 0 ");
        printTree(root->right, prefix + " 1 ");
    }
}

Node* buildTree(const unordered_map<char,int>& freq) {
    priority_queue<Node*, vector<Node*>, comp> pq;
    for (auto& p : freq)
        pq.push(new Node(p.first, p.second));
    while (pq.size() > 1) {
        Node* left = pq.top(); pq.pop();
        Node* right = pq.top(); pq.pop();
        pq.push(new Node('\0', left->freq + right->freq, left, right));
    }
    return pq.top();
}

void buildCodes(Node* root, const string& prefix, unordered_map<char,string>& codes) {
    if (!root) return;
    if (!root->left && !root->right) {
        codes[root->ch] = prefix;
    }
    buildCodes(root->left, prefix + "0", codes);
    buildCodes(root->right, prefix + "1", codes);
}

void deleteTree(Node* root) {
    if (!root) return;
    deleteTree(root->left);
    deleteTree(root->right);
    delete root;
}

void saveCodes(ofstream& out, const unordered_map<char,string>& codes) {
    size_t count = codes.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (auto& p : codes) {
        char c = p.first;
        out.write(&c, sizeof(c));
        size_t len = p.second.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(p.second.data(), len);
    }
}

// Возвращает таблицу кодов из файла
unordered_map<char,string> loadCodes(ifstream& in) {
    unordered_map<char,string> codes;
    size_t count;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    for (size_t i = 0; i < count; i++) {
        char c;
        in.read(&c, sizeof(c));
        size_t len;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        string code(len, ' ');
        in.read(&code[0], len);
        codes[c] = code;
    }
    return codes;
}

// Класс для записи битов в файл эффективно
class BitWriter {
    ofstream& out;
    uint8_t buffer = 0;
    int bitsFilled = 0;
public:
    BitWriter(ofstream& os) : out(os) {}
    void writeBit(bool bit) {
        buffer <<= 1;
        if (bit) buffer |= 1;
        bitsFilled++;
        if (bitsFilled == 8) {
            out.put(buffer);
            buffer = 0;
            bitsFilled = 0;
        }
    }
    void writeBits(const string& bits) {
        for (char c : bits) {
            writeBit(c == '1');
        }
    }
    void flush() {
        if (bitsFilled > 0) {
            buffer <<= (8 - bitsFilled);
            out.put(buffer);
            buffer = 0;
            bitsFilled = 0;
        }
    }
};

// Класс для чтения битов из файла эффективно
class BitReader {
    ifstream& in;
    uint8_t buffer = 0;
    int bitsLeft = 0;
public:
    BitReader(ifstream& is) : in(is) {}
    bool readBit(bool& bit) {
        if (bitsLeft == 0) {
            int val = in.get();
            if (val == EOF) return false;
            buffer = (uint8_t)val;
            bitsLeft = 8;
        }
        bit = (buffer & (1 << (bitsLeft - 1))) != 0;
        bitsLeft--;
        return true;
    }
};

void encodeFile(const string& inputFile, const string& outputFile) {
    ifstream in(inputFile, ios::binary);
    if (!in) {
        cerr << "Cannot open input file\n";
        exit(1);
    }
    string text((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());

    unordered_map<char,int> freq;
    for (char c : text)
        freq[c]++;

    Node* root = buildTree(freq);

    cout << "Huffman Tree:\n";
    printTree(root);

    unordered_map<char,string> codes;
    buildCodes(root, "", codes);

    deleteTree(root);

    ofstream out(outputFile, ios::binary);
    if (!out) {
        cerr << "Cannot open output file\n";
        exit(1);
    }

    // Записать таблицу кодов
    saveCodes(out, codes);

    // Записать длину битовой последовательности
    size_t totalBits = 0;
    for (char c : text)
        totalBits += codes[c].size();
    out.write(reinterpret_cast<const char*>(&totalBits), sizeof(totalBits));

    // Записать сжатые данные
    BitWriter bw(out);
    for (char c : text) {
        bw.writeBits(codes[c]);
    }
    bw.flush();

    cout << "Encoding done. Original size: " << text.size() << " bytes, Compressed size: " << out.tellp() << " bytes\n";
}

void decodeFile(const string& inputFile, const string& outputFile) {
    ifstream in(inputFile, ios::binary);
    if (!in) {
        cerr << "Cannot open input file\n";
        exit(1);
    }

    unordered_map<char,string> codes = loadCodes(in);

    size_t totalBits;
    in.read(reinterpret_cast<char*>(&totalBits), sizeof(totalBits));

    // Для декодирования удобнее построить дерево из таблицы кодов
    // Вот функция построения дерева из кодов:

    Node* root = new Node('\0', 0);
    for (auto& p : codes) {
        Node* current = root;
        const string& code = p.second;
        for (char bit : code) {
            if (bit == '0') {
                if (!current->left)
                    current->left = new Node('\0', 0);
                current = current->left;
            } else {
                if (!current->right)
                    current->right = new Node('\0', 0);
                current = current->right;
            }
        }
        current->ch = p.first;
    }

    ofstream out(outputFile, ios::binary);
    if (!out) {
        cerr << "Cannot open output file\n";
        exit(1);
    }

    BitReader br(in);
    Node* current = root;
    for (size_t i = 0; i < totalBits; i++) {
        bool bit;
        if (!br.readBit(bit)) {
            cerr << "Unexpected EOF while reading bits\n";
            break;
        }
        if (bit) current = current->right;
        else current = current->left;

        if (!current->left && !current->right) {
            out.put(current->ch);
            current = root;
        }
    }

    cout << "Decoding done.\n";

    deleteTree(root);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "Usage:\n";
        cout << argv[0] << " encode <input_file> <output_file>\n";
        cout << argv[0] << " decode <input_file> <output_file>\n";
        return 1;
    }

    string mode = argv[1];
    string inputFile = argv[2];
    string outputFile = argv[3];

    if (mode == "encode") {
        encodeFile(inputFile, outputFile);
    } else if (mode == "decode") {
        decodeFile(inputFile, outputFile);
    } else {
        cerr << "Unknown mode: " << mode << "\n";
        return 1;
    }

    return 0;
}
