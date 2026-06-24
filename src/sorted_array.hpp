#pragma once
#include <fstream>
#include <string>
#include <tuple>

// Arquivo ordenado em disco — baseline "menos eficiente" para comparar com a
// Árvore B. Modelo de I/O: 1 chave por bloco/página. Cada leitura ou escrita de
// uma chave conta como 1 acesso a disco, espelhando o contador do DiskManager da
// B-tree (o cabeçalho, slot 0, NÃO conta — igual ao readHeader/writeHeader).
//
// Layout do arquivo:
//   slot 0      -> cabeçalho: quantidade de chaves (count)
//   slot 1..n   -> chaves em ordem crescente, uma por slot
//
// Trade-off demonstrado:
//   - search()  -> busca binária: O(log n) acessos (rápido, como a B-tree)
//   - insert()  -> busca binária + deslocamento O(n) de escritas (péssimo)
class SortedArray {
public:
    explicit SortedArray(const std::string& path);
    ~SortedArray();

    // Busca binária. Retorna (índice da chave, achou).
    std::tuple<int,bool> search(int x);

    // Inserção mantendo a ordem: localiza a posição e desloca o resto à direita.
    void insert(int x);

    // Remoção mantendo a ordem: localiza a chave e desloca o resto à esquerda.
    // Retorna true se a chave existia e foi removida.
    bool remove(int x);

    int count();

    int  diskAccesses() const { return accesses_; }
    void resetCounter()       { accesses_ = 0; }

private:
    std::fstream file_;
    int          accesses_{0};

    static constexpr int SLOT_SIZE = static_cast<int>(sizeof(int));

    std::streamoff slotOffset(int slot) const;

    // chave lógica i (0-based) -> slot físico i+1; cada acesso incrementa o contador
    int  readKey (int i);
    void writeKey(int i, int v);

    // cabeçalho: não conta como acesso a disco (igual ao header da B-tree)
    int  readCount();
    void writeCount(int c);
};
