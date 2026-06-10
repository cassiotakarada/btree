#pragma once
#include <cstring>

#ifndef M
  #define M 5
#endif

static_assert(M >= 3,
    "M deve ser >= 3 (B-tree de ordem 2 ou menor é degenerada e o split "
    "produziria nós com 0 chaves). Recompile com `make M=3` ou maior.");

constexpr int ORDER    = M;
constexpr int MAX_KEYS = M - 1;
constexpr int MID      = (M + 1) / 2;
constexpr int PAGE_SIZE = static_cast<int>(sizeof(int) * (1 + M + M));

struct BNode {
    int n;
    int A[M + 1];
    int K[M + 1];
};

struct Header {
    int root;
    int total;
    int free_head;
};
