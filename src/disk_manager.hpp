#pragma once
#include <fstream>
#include <string>
#include "types.hpp"

class DiskManager {
public:
    explicit DiskManager(const std::string& path);
    ~DiskManager();

    BNode  readNode (int rrn);
    void   writeNode(int rrn, const BNode& node);

    // Allocates a new RRN (from free list or by appending).
    // Does NOT write to disk — caller must writeNode afterwards.
    int    allocNode();
    // Pushes rrn onto the free list and persists the header.
    void   freeNode (int rrn);

    Header readHeader ();
    void   writeHeader(const Header& h);

    int  diskAccesses() const { return accesses_; }
    void resetCounter()       { accesses_ = 0; }

    // Liga/desliga o reaproveitamento de nós (free list). Quando desligado,
    // allocNode SEMPRE acrescenta um novo registro ao final do arquivo, mesmo
    // que haja nós liberados — útil para medir a ocupação do arquivo "sem
    // reaproveitamento". Ligado por padrão.
    void setReuse(bool on) { reuse_ = on; }
    bool reuse() const     { return reuse_; }

    // Tamanho atual do arquivo binário em bytes (registro 0 = header incluso).
    std::streamoff fileSizeBytes();

    // Métricas de ocupação derivadas do header (não contam como acesso a disco).
    int totalNodes();   // maior RRN já alocado (registros físicos no arquivo)
    int freeNodes();    // quantidade de nós atualmente na free list

private:
    std::fstream file_;
    int          accesses_{0};
    bool         reuse_{true};

    std::streamoff nodeOffset(int rrn) const;
};
