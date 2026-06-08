#pragma once
#include <cstring>

#ifndef M
  #define M 5
#endif

// B-tree de ordem 2 é degenerada: o split divide um nó cheio (n=1) em
//   ⌈m/2⌉−1 = 0 chaves à esquerda + 1 chave à direita + mediana promovida.
// Isso deixa a metade esquerda VAZIA (zumbi), invalidando a árvore inteira.
// Ordens 1 e 0 sequer fazem sentido. Exigimos M >= 3 em tempo de compilação.
static_assert(M >= 3,
    "M deve ser >= 3 (B-tree de ordem 2 ou menor é degenerada e o split "
    "produziria nós com 0 chaves). Recompile com `make M=3` ou maior.");

constexpr int ORDER    = M;
constexpr int MAX_KEYS = M - 1;
constexpr int MID      = (M + 1) / 2;  // ceil(m/2)
// n, A[0..M-1], K[0..M-1]  — all ints
constexpr int PAGE_SIZE = static_cast<int>(sizeof(int) * (1 + M + M));

// Disk node: professor's notation n, A0, (K1,A1), ..., (Kn,An)
// K[0] is unused padding so indices match 1-based slide notation.
// Arrays are M+1 to absorb one temporary overflow during insertB
// (before splitNode runs). Disk I/O only ever reads/writes M ints per array.
struct BNode {
    int n;        // current key count
    int A[M + 1]; // child RRNs; A[0..n] valid; A[M] is scratch
    int K[M + 1]; // keys; K[1..n] valid; K[0] = pad; K[M] is scratch
};

// Record 0 in the binary file: tree metadata
struct Header {
    int root;       // RRN of root node; 0 = empty tree
    int total;      // total nodes ever written
    int free_head;  // head of free-node list; -1 = empty
};
