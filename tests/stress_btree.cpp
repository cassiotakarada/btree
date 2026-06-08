// Teste de stress / regressão para a B-tree.
// Constrói árvores aleatórias, remove em ordem aleatória, e em cada passo
// valida 5 invariantes da estrutura. Não usa o menu interativo: chama
// BTree direto.
//
// Como rodar:
//   make tests && ./tests/stress_btree
//
// Em qualquer falha: imprime o estado completo, o cenário (seed, N) e aborta.
#include <iostream>
#include <random>
#include <vector>
#include <set>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include "../src/disk_manager.hpp"
#include "../src/btree.hpp"

namespace {

struct Validator {
    DiskManager& dm;
    std::set<int> seenRRN;       // detecta ciclo / aliasing
    std::set<int> seenKey;       // detecta duplicata
    int           leafDepth = -1;

    explicit Validator(DiskManager& d) : dm(d) {}

    void fail(const std::string& msg, int rrn = -1) {
        std::cerr << "INVARIANTE VIOLADA";
        if (rrn >= 0) std::cerr << " (no RRN " << rrn << ")";
        std::cerr << ": " << msg << "\n";
        std::exit(1);
    }

    // Recursivo: valida cada nó e propaga (lo, hi) com a faixa permitida.
    int walk(int rrn, int lo, int hi, int depth) {
        if (rrn == 0) return depth;

        if (!seenRRN.insert(rrn).second)
            fail("RRN visitado duas vezes (ciclo ou aliasing)", rrn);

        BNode node = dm.readNode(rrn);

        if (node.n <= 0)
            fail("nó alcançado pela árvore com n<=0 (provavelmente da free list)", rrn);
        if (node.n > MAX_KEYS)
            fail("nó com n > MAX_KEYS", rrn);

        for (int i = 1; i <= node.n; ++i) {
            int k = node.K[i];

            if (k <= lo || k >= hi)
                fail("chave fora da faixa permitida pela invariante de B-tree", rrn);

            if (i >= 2 && node.K[i] <= node.K[i - 1])
                fail("chaves não estritamente crescentes dentro do nó", rrn);

            if (!seenKey.insert(k).second)
                fail("chave duplicada na árvore", rrn);
        }

        bool isLeaf = (node.A[0] == 0);
        if (isLeaf) {
            if (leafDepth == -1) leafDepth = depth;
            else if (depth != leafDepth)
                fail("folhas em profundidades diferentes (árvore não-balanceada)", rrn);
            return depth;
        }

        // Não-folha: cada A[i] deve ser != 0 para i in 0..n
        for (int i = 0; i <= node.n; ++i) {
            if (node.A[i] == 0)
                fail("nó interno com filho ausente", rrn);
        }

        for (int i = 0; i <= node.n; ++i) {
            int childLo = (i == 0)        ? lo : node.K[i];
            int childHi = (i == node.n)   ? hi : node.K[i + 1];
            walk(node.A[i], childLo, childHi, depth + 1);
        }
        return leafDepth;
    }

    // Validação completa + comparação com `expected` (conjunto canônico).
    void validate(const std::set<int>& expected, const std::string& tag) {
        seenRRN.clear();
        seenKey.clear();
        leafDepth = -1;

        Header hdr = dm.readHeader();
        if (hdr.root != 0) walk(hdr.root, INT_MIN, INT_MAX, 0);

        if (seenKey != expected) {
            std::cerr << "INVARIANTE VIOLADA: conjunto de chaves divergente em " << tag << "\n"
                      << "  esperado: " << expected.size() << " chaves\n"
                      << "  na árvore: " << seenKey.size() << " chaves\n";
            // diferenças
            std::set<int> faltam, sobram;
            std::set_difference(expected.begin(), expected.end(),
                                seenKey.begin(),  seenKey.end(),
                                std::inserter(faltam, faltam.begin()));
            std::set_difference(seenKey.begin(),  seenKey.end(),
                                expected.begin(), expected.end(),
                                std::inserter(sobram, sobram.begin()));
            std::cerr << "  faltando: ";
            for (int k : faltam) std::cerr << k << " ";
            std::cerr << "\n  sobrando: ";
            for (int k : sobram) std::cerr << k << " ";
            std::cerr << "\n";
            std::exit(1);
        }
    }
};

void runScenario(const std::string& binPath, unsigned seed, int N, bool verbose) {
    std::remove(binPath.c_str());
    DiskManager dm(binPath);
    BTree bt(dm);

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(1, 10 * N);

    // Gera N chaves *únicas* (usa set pra evitar duplicatas; insertB já trata,
    // mas pra simplificar a validação manter o conjunto canônico mais limpo).
    std::set<int> expected;
    std::vector<int> order;
    while ((int)expected.size() < N) {
        int k = dist(rng);
        if (expected.insert(k).second) order.push_back(k);
    }

    Validator v(dm);

    if (verbose) std::cout << "  inserindo " << N << " chaves...\n";
    int inserted = 0;
    for (int k : order) {
        bt.insertB(k);
        ++inserted;
        // valida só periodicamente pra não explodir o tempo
        if (inserted == N || inserted % std::max(1, N / 10) == 0) {
            std::set<int> sofar(order.begin(), order.begin() + inserted);
            v.validate(sofar, "após inserir " + std::to_string(inserted));
        }
    }

    if (verbose) {
        Header h = dm.readHeader();
        std::cout << "  altura final = " << bt.height()
                  << ", total nós alocados = " << h.total << "\n";
    }

    // Remove em ordem embaralhada
    std::vector<int> delOrder = order;
    std::shuffle(delOrder.begin(), delOrder.end(), std::mt19937(seed ^ 0xDEADBEEF));

    if (verbose) std::cout << "  removendo " << N << " chaves em ordem embaralhada...\n";
    int removed = 0;
    std::set<int> remaining = expected;
    for (int k : delOrder) {
        bt.deleteB(k);
        remaining.erase(k);
        ++removed;
        if (removed == N || removed % std::max(1, N / 10) == 0) {
            v.validate(remaining,
                       "após remover " + std::to_string(removed) + " (ultima=" + std::to_string(k) + ")");
        }
    }

    Header h = dm.readHeader();
    if (h.root != 0) {
        std::cerr << "INVARIANTE VIOLADA: raiz != 0 depois de remover todas as chaves "
                  << "(root = " << h.root << ")\n";
        std::exit(1);
    }
    if (verbose) std::cout << "  OK ✓\n";
}

}  // namespace

int main() {
    struct Case { unsigned seed; int N; };
    std::vector<Case> cases = {
        {1,    20},
        {2,    50},
        {7,    21},     // o caso histórico
        {42,   200},
        {100,  500},
        {2026, 1000},
        {99,   2000},
    };

    for (auto& c : cases) {
        std::cout << "[seed=" << c.seed << ", N=" << c.N << "]\n";
        runScenario("/tmp/stress_btree.bin", c.seed, c.N, true);
    }

    std::remove("/tmp/stress_btree.bin");
    std::cout << "\nTodos os " << cases.size() << " cenários passaram.\n";
    return 0;
}
