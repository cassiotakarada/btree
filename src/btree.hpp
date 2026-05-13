#pragma once
#include <tuple>
#include <vector>
#include "disk_manager.hpp"

class BTree {
public:
    explicit BTree(DiskManager& dm);

    // Busca m-way: percorre da raiz à folha; retorna (RRN do nó, índice i, achou).
    // Se achou == true, K[i] == x. Caso contrário, x deveria ficar entre K[i] e K[i+1]
    // no último nó visitado (folha ou árvore vazia) — útil para inserção.
    std::tuple<int,int,bool> mSearch(int x);

    // Inserção bottom-up: localiza folha com mSearchPath, insere e propaga splits.
    void insertB(int x);

    // Remoção: troca por sucessor se não for folha; remove na folha; repara underflow
    // (redistribuição com irmão ou fusão) subindo até a raiz.
    void deleteB(int x);

    void printTree();   // hierarchical indented print

    int  diskAccesses() const { return dm_.diskAccesses(); }
    void resetCounter()       { dm_.resetCounter(); }

private:
    DiskManager& dm_;

    // Igual a mSearch, mas preenche path com os RRNs dos ancestrais (para insert/delete).
    std::tuple<int,int,bool> mSearchPath(int x, std::vector<int>& path);

    // Insere chave k e ponteiro filho à direita a (subárvore nova após split) em ordem.
    void insertInNode(BNode& p, int k, int a);

    // Divide nó cheio em dois: esquerda mantém chaves < mediana, direita o restante;
    // mediana sobe para o pai. Retorna RRN do nó direito novo.
    int splitNode(int pRRN, BNode& p, int& mediana);

    // Remove a chave no índice i (1-based) e desloca A[i] junto (filho à direita da chave).
    void removeFromNode(BNode& p, int i);

    // Desce pelo ponteiro A[0] até a folha mais à esquerda da subárvore (sucessor in-order).
    int findSuccessorLeaf(int childRRN, std::vector<int>& path);

    void printNode(int rrn, int depth);
};
