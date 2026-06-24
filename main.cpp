#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <random>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include "src/disk_manager.hpp"
#include "src/btree.hpp"
#include "src/sorted_array.hpp"

struct ScopedTimer {
    using clock = std::chrono::high_resolution_clock;
    clock::time_point t0;
    double& cumulative;

    explicit ScopedTimer(double& c) : t0(clock::now()), cumulative(c) {}

    ~ScopedTimer() {
        auto t1 = clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        cumulative += ms;
        std::cout << std::fixed << std::setprecision(3)
                  << "Tempo: " << ms << " ms\n";
    }
};

static void showMenu() {
    std::cout << "\n=== B-Tree (ordem " << ORDER << ") ===\n"
              << "1. Inserir chave\n"
              << "2. Buscar chave\n"
              << "3. Remover chave\n"
              << "4. Acessos ao disco e tempo de execução\n"
              << "5. Outras opções\n"
              << "0. Sair\n"
              << "> ";
}

static void showOtherOptionsMenu() {
    std::cout << "\n--- Outras opções (B-Tree ordem " << ORDER << ") ---\n"
              << "1. Imprimir árvore\n"
              << "2. Zerar contador\n"
              << "3. Gerar árvore aleatória (altura mínima)\n"
              << "4. Experimento aleatório\n"
              << "5. Exportar para Graphviz (.dot)\n"
              << "6. Comparar com Array Ordenado (busca binária)\n"
              << "7. Experimento aleatório (Array Ordenado)\n"
              << "0. Voltar ao menu principal\n"
              << "> ";
}

static void runGenerate(BTree& bt) {
    int targetH;
    std::cout << "Altura mínima desejada: ";
    std::cin >> targetH;

    unsigned seed;
    std::cout << "Seed (0 = aleatório por tempo): ";
    std::cin >> seed;
    if (seed == 0)
        seed = static_cast<unsigned>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(1, 9999);

    std::vector<int> inserted;
    int h = bt.height();
    int safety = 100000;
    while (h < targetH && safety-- > 0) {
        int k = dist(rng);
        int before = bt.height();
        bt.insertB(k);
        int after = bt.height();
        if (after != before || (int)inserted.size() < 200)
            inserted.push_back(k);
        h = after;
    }

    std::cout << "\nÁrvore gerada (seed=" << seed << ", altura=" << h
              << ", chaves inseridas=" << inserted.size() << "):\n";
    bt.printTree();

    std::cout << "\nAlgumas chaves para testar mSearch (busca):\n  ";
    int shown = 0;
    for (int k : inserted) {
        std::cout << k << " ";
        if (++shown >= 20) break;
    }
    std::cout << "\n";
}

static double runExperiment(const std::string& basePath) {
    int N;
    std::cout << "Quantidade de chaves N: ";
    std::cin >> N;
    int mode;
    std::cout << "Modo (0=aleatorio, 1=sequencial): ";
    std::cin >> mode;

    std::string path = basePath + "_exp.bin";
    DiskManager dm(path);
    BTree bt(dm);

    std::vector<int> keys(N);
    if (mode == 0) {
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> dist(1, N * 10);
        for (auto& k : keys) k = dist(rng);
    } else {
        for (int i = 0; i < N; ++i) keys[i] = i + 1;
    }

    dm.resetCounter();
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int k : keys) bt.insertB(k);
    auto t1 = std::chrono::high_resolution_clock::now();

    long insAcc = dm.diskAccesses();
    double insMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    dm.resetCounter();
    t0 = std::chrono::high_resolution_clock::now();
    for (int k : keys) bt.mSearch(k);
    t1 = std::chrono::high_resolution_clock::now();

    long srchAcc = dm.diskAccesses();
    double srchMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    dm.resetCounter();
    std::shuffle(keys.begin(), keys.end(), std::mt19937(99));
    t0 = std::chrono::high_resolution_clock::now();
    for (int k : keys) bt.deleteB(k);
    t1 = std::chrono::high_resolution_clock::now();

    long delAcc = dm.diskAccesses();
    double delMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    double totalMs = insMs + srchMs + delMs;

    std::cout << std::fixed << std::setprecision(3)
              << "\n--- Resultados (ordem=" << ORDER << ", N=" << N
              << ", modo=" << (mode ? "seq" : "rand") << ") ---\n"
              << "Inserção : " << insAcc  << " acessos  ("
              << insAcc / (double)N  << " médio/op)  " << insMs  << " ms\n"
              << "Busca    : " << srchAcc << " acessos  ("
              << srchAcc / (double)N << " médio/op)  " << srchMs << " ms\n"
              << "Remoção  : " << delAcc  << " acessos  ("
              << delAcc / (double)N  << " médio/op)  " << delMs  << " ms\n"
              << "Total    : " << totalMs << " ms\n";

    return totalMs;
}

// Comparação justa Árvore B x Array Ordenado em disco: mesmas chaves, mesmo
// contador de acessos (1 registro lido/escrito = 1 acesso). Mostra o trade-off:
// a busca binária do array quase empata, mas a inserção (deslocamento O(n)) explode.
static void runCompareSortedArray(const std::string& basePath) {
    int N;
    std::cout << "Quantidade de chaves N: ";
    std::cin >> N;
    if (N <= 0) { std::cout << "N invalido.\n"; return; }

    std::vector<int> keys(N);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, N * 10);
    for (auto& k : keys) k = dist(rng);

    // --- Árvore B ---
    std::string btPath = basePath + "_cmp_btree.bin";
    std::remove(btPath.c_str());
    DiskManager dm(btPath);
    BTree bt(dm);

    dm.resetCounter();
    for (int k : keys) bt.insertB(k);
    long btIns = dm.diskAccesses();

    dm.resetCounter();
    for (int k : keys) bt.mSearch(k);
    long btSrch = dm.diskAccesses();

    std::vector<int> delOrder = keys;
    std::shuffle(delOrder.begin(), delOrder.end(), std::mt19937(99));

    dm.resetCounter();
    for (int k : delOrder) bt.deleteB(k);
    long btDel = dm.diskAccesses();

    // --- Array Ordenado ---
    std::string saPath = basePath + "_cmp_sorted.bin";
    std::remove(saPath.c_str());
    SortedArray sa(saPath);

    sa.resetCounter();
    for (int k : keys) sa.insert(k);
    long saIns = sa.diskAccesses();

    sa.resetCounter();
    for (int k : keys) sa.search(k);
    long saSrch = sa.diskAccesses();

    sa.resetCounter();
    for (int k : delOrder) sa.remove(k);
    long saDel = sa.diskAccesses();

    std::cout << std::fixed << std::setprecision(3)
              << "\n=== Árvore B (ordem " << ORDER << ") x Array Ordenado  (N=" << N << ") ===\n"
              << "                        | acessos totais | médio por operação\n"
              << "------------------------+----------------+-------------------\n"
              << "Inserção  | Árvore B    | " << std::setw(14) << btIns
              << " | " << btIns / (double)N << "\n"
              << "          | Array Ord.  | " << std::setw(14) << saIns
              << " | " << saIns / (double)N << "\n"
              << "------------------------+----------------+-------------------\n"
              << "Busca     | Árvore B    | " << std::setw(14) << btSrch
              << " | " << btSrch / (double)N << "\n"
              << "          | Array Ord.  | " << std::setw(14) << saSrch
              << " | " << saSrch / (double)N << "\n"
              << "------------------------+----------------+-------------------\n"
              << "Remoção   | Árvore B    | " << std::setw(14) << btDel
              << " | " << btDel / (double)N << "\n"
              << "          | Array Ord.  | " << std::setw(14) << saDel
              << " | " << saDel / (double)N << "\n"
              << "------------------------+----------------+-------------------\n"
              << "Inserção: Array é " << (btIns ? saIns / (double)btIns : 0.0)
              << "x mais cara que a Árvore B (deslocamento O(n)).\n"
              << "Busca   : Array é " << (btSrch ? saSrch / (double)btSrch : 0.0)
              << "x a da Árvore B (ambas O(log n)).\n"
              << "Remoção : Array é " << (btDel ? saDel / (double)btDel : 0.0)
              << "x mais cara que a Árvore B (deslocamento O(n)).\n";

    std::remove(btPath.c_str());
    std::remove(saPath.c_str());
}

// Experimento aleatório só com o Array Ordenado: insere N chaves aleatórias e
// busca todas, reportando acessos a disco (médio/op) e tempo de CPU.
static void runSortedArrayExperiment(const std::string& basePath) {
    int N;
    std::cout << "Quantidade de chaves N: ";
    std::cin >> N;
    if (N <= 0) { std::cout << "N invalido.\n"; return; }

    std::vector<int> keys(N);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, N * 10);
    for (auto& k : keys) k = dist(rng);

    std::string path = basePath + "_sorted_exp.bin";
    std::remove(path.c_str());
    SortedArray sa(path);

    sa.resetCounter();
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int k : keys) sa.insert(k);
    auto t1 = std::chrono::high_resolution_clock::now();
    long insAcc = sa.diskAccesses();
    double insMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    sa.resetCounter();
    t0 = std::chrono::high_resolution_clock::now();
    for (int k : keys) sa.search(k);
    t1 = std::chrono::high_resolution_clock::now();
    long srchAcc = sa.diskAccesses();
    double srchMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    sa.resetCounter();
    std::shuffle(keys.begin(), keys.end(), std::mt19937(99));
    t0 = std::chrono::high_resolution_clock::now();
    for (int k : keys) sa.remove(k);
    t1 = std::chrono::high_resolution_clock::now();
    long delAcc = sa.diskAccesses();
    double delMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << std::fixed << std::setprecision(3)
              << "\n--- Array Ordenado (N=" << N << ", modo=rand) ---\n"
              << "Inserção : " << insAcc  << " acessos  ("
              << insAcc / (double)N  << " médio/op)  " << insMs  << " ms\n"
              << "Busca    : " << srchAcc << " acessos  ("
              << srchAcc / (double)N << " médio/op)  " << srchMs << " ms\n"
              << "Remoção  : " << delAcc  << " acessos  ("
              << delAcc / (double)N  << " médio/op)  " << delMs  << " ms\n"
              << "Total    : " << insMs + srchMs + delMs << " ms\n";

    std::remove(path.c_str());
}

static void runOtherOptions(BTree& bt, const std::string& path, double& cumulativeMs) {
    int opt;
    do {
        showOtherOptionsMenu();
        std::cin >> opt;
        if (opt == 1) {
            ScopedTimer timer(cumulativeMs);
            bt.printTree();
        } else if (opt == 2) {
            bt.resetCounter();
            cumulativeMs = 0.0;
            std::cout << "Contador zerado (acessos e tempo).\n";
        } else if (opt == 3) {
            runGenerate(bt);
        } else if (opt == 4) {
            cumulativeMs += runExperiment(path);
        } else if (opt == 5) {
            const std::string dotPath = "tree.dot";
            const std::string pngPath = "tree.png";
            bt.exportDot(dotPath);
            std::cout << "Gerado: " << dotPath << "\n";

            std::string cmd = "dot -Tpng -Gdpi=120 " + dotPath + " -o " + pngPath;
            int rc = std::system(cmd.c_str());
            if (rc == 0) {
                std::cout << "Gerado: " << pngPath << "\n";
            } else {
                std::cout << "Falha ao executar 'dot' (rc=" << rc << ").\n"
                          << "Instale com: sudo apt install graphviz\n"
                          << "Ou cole o conteúdo de " << dotPath
                          << " em https://dreampuf.github.io/GraphvizOnline\n";
            }
        } else if (opt == 6) {
            runCompareSortedArray(path);
        } else if (opt == 7) {
            runSortedArrayExperiment(path);
        }
    } while (opt != 0);
}

int main(int argc, char* argv[]) {
    std::string path = (argc > 1) ? argv[1] : "btree.bin";
    DiskManager dm(path);
    BTree bt(dm);

    double cumulativeMs = 0.0;

    int opt;
    do {
        showMenu();
        std::cin >> opt;
        if (opt == 1) {
            int k; std::cout << "Chave: "; std::cin >> k;
            ScopedTimer timer(cumulativeMs);
            auto [pPre, iPre, exists] = bt.mSearch(k);
            if (exists) {
                std::cout << "Chave " << k << " já existe no RRN " << pPre
                          << " (posição " << iPre << ").\n";
            } else {
                bt.insertB(k);
                auto [pPos, iPos, ok] = bt.mSearch(k);
                if (ok)
                    std::cout << "Inserido no RRN " << pPos
                              << " (posição " << iPos << ").\n";
                else
                    std::cout << "Inserido.\n";
            }
        } else if (opt == 2) {
            int k; std::cout << "Chave: "; std::cin >> k;
            ScopedTimer timer(cumulativeMs);
            auto [p, i, found] = bt.mSearch(k);
            if (found)
                std::cout << "Encontrado no RRN " << p << " posição " << i << ".\n";
            else
                std::cout << "Não encontrado.\n";
        } else if (opt == 3) {
            int k; std::cout << "Chave: "; std::cin >> k;
            ScopedTimer timer(cumulativeMs);
            auto [p, i, found] = bt.mSearch(k);
            if (!found) {
                std::cout << "Essa chave não existe.\n";
            } else {
                bt.deleteB(k);
                std::cout << "Removido do RRN " << p
                          << " (posição " << i << ").\n";
            }
        } else if (opt == 4) {
            std::cout << "Acessos ao disco: " << bt.diskAccesses() << "\n"
                      << std::fixed << std::setprecision(3)
                      << "Tempo de execução acumulado: " << cumulativeMs << " ms\n";
        } else if (opt == 5) {
            runOtherOptions(bt, path, cumulativeMs);
        }
    } while (opt != 0);

    return 0;
}
