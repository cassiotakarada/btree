#include "sorted_array.hpp"

SortedArray::SortedArray(const std::string& path) {
    file_.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
        file_.open(path, std::ios::in | std::ios::out |
                         std::ios::binary | std::ios::trunc);
        writeCount(0);
    }
}

SortedArray::~SortedArray() {
    if (file_.is_open()) file_.close();
}

std::streamoff SortedArray::slotOffset(int slot) const {
    return static_cast<std::streamoff>(slot) * SLOT_SIZE;
}

int SortedArray::readKey(int i) {
    ++accesses_;                 // chave i -> slot físico i+1
    int v = 0;
    file_.clear();
    file_.seekg(slotOffset(i + 1));
    file_.read(reinterpret_cast<char*>(&v), SLOT_SIZE);
    return v;
}

void SortedArray::writeKey(int i, int v) {
    ++accesses_;
    file_.clear();
    file_.seekp(slotOffset(i + 1));
    file_.write(reinterpret_cast<const char*>(&v), SLOT_SIZE);
    file_.flush();
}

int SortedArray::readCount() {
    int c = 0;
    file_.clear();
    file_.seekg(slotOffset(0));
    file_.read(reinterpret_cast<char*>(&c), SLOT_SIZE);
    return c;
}

void SortedArray::writeCount(int c) {
    file_.clear();
    file_.seekp(slotOffset(0));
    file_.write(reinterpret_cast<const char*>(&c), SLOT_SIZE);
    file_.flush();
}

int SortedArray::count() {
    return readCount();
}

// Busca binária — O(log n) leituras de chave.
std::tuple<int,bool> SortedArray::search(int x) {
    int c = readCount();
    int lo = 0, hi = c - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int v = readKey(mid);
        if (v == x) return {mid, true};
        if (v < x)  lo = mid + 1;
        else        hi = mid - 1;
    }
    return {lo, false};          // lo = posição de inserção
}

// Inserção: localiza a posição (busca binária) e desloca tudo à direita.
// O deslocamento é O(n) escritas — o gargalo que a Árvore B evita.
void SortedArray::insert(int x) {
    int c = readCount();

    // posição de inserção via busca binária (leituras O(log n))
    int lo = 0, hi = c - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int v = readKey(mid);
        if (v < x) lo = mid + 1;
        else       hi = mid - 1;
    }
    int pos = lo;

    // desloca [pos, c-1] uma casa à direita: para cada elemento, 1 leitura + 1 escrita
    for (int i = c - 1; i >= pos; --i) {
        int v = readKey(i);
        writeKey(i + 1, v);
    }

    writeKey(pos, x);
    writeCount(c + 1);
}

// Remoção: localiza a chave (busca binária) e desloca [pos+1, c-1] à esquerda.
// O deslocamento é O(n) escritas — mesmo gargalo da inserção.
bool SortedArray::remove(int x) {
    int c = readCount();

    // localiza a chave via busca binária (leituras O(log n))
    int lo = 0, hi = c - 1, pos = -1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int v = readKey(mid);
        if (v == x) { pos = mid; break; }
        if (v < x)  lo = mid + 1;
        else        hi = mid - 1;
    }
    if (pos < 0) return false;          // chave não existe

    // desloca [pos+1, c-1] uma casa à esquerda: para cada elemento, 1 leitura + 1 escrita
    for (int i = pos + 1; i < c; ++i) {
        int v = readKey(i);
        writeKey(i - 1, v);
    }

    writeCount(c - 1);
    return true;
}
