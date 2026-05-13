#include <iostream>
#include <string>
#include <chrono>
#include <random>
#include <algorithm>
#include <vector>
#include "src/disk_manager.hpp"
#include "src/btree.hpp"

static void showMenu() {
    std::cout << "\n=== B-Tree (ordem " << ORDER << ") ===\n"
              << "1. Inserir chave\n"
              << "2. Buscar chave\n"
              << "3. Remover chave\n"
              << "4. Imprimir árvore\n"
              << "5. Acessos ao disco\n"
              << "6. Zerar contador\n"
              << "7. Experimento automático\n"
              << "0. Sair\n"
              << "> ";
}

static void runExperiment(const std::string& basePath) {
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

    std::cout << "\n--- Resultados (ordem=" << ORDER << ", N=" << N
              << ", modo=" << (mode ? "seq" : "rand") << ") ---\n"
              << "Inserção : " << insAcc  << " acessos  ("
              << insAcc / (double)N  << " médio/op)  " << insMs  << " ms\n"
              << "Busca    : " << srchAcc << " acessos  ("
              << srchAcc / (double)N << " médio/op)  " << srchMs << " ms\n"
              << "Remoção  : " << delAcc  << " acessos  ("
              << delAcc / (double)N  << " médio/op)  " << delMs  << " ms\n";
}

int main(int argc, char* argv[]) {
    std::string path = (argc > 1) ? argv[1] : "btree.bin";
    DiskManager dm(path);
    BTree bt(dm);

    int opt;
    do {
        showMenu();
        std::cin >> opt;
        if (opt == 1) {
            int k; std::cout << "Chave: "; std::cin >> k;
            bt.insertB(k);
            std::cout << "Inserido.\n";
        } else if (opt == 2) {
            int k; std::cout << "Chave: "; std::cin >> k;
            auto [p, i, found] = bt.mSearch(k);
            if (found)
                std::cout << "Encontrado no RRN " << p << " posição " << i << ".\n";
            else
                std::cout << "Não encontrado.\n";
        } else if (opt == 3) {
            int k; std::cout << "Chave: "; std::cin >> k;
            bt.deleteB(k);
            std::cout << "Removido (se existia).\n";
        } else if (opt == 4) {
            bt.printTree();
        } else if (opt == 5) {
            std::cout << "Acessos ao disco: " << bt.diskAccesses() << "\n";
        } else if (opt == 6) {
            bt.resetCounter();
            std::cout << "Contador zerado.\n";
        } else if (opt == 7) {
            runExperiment(path);
        }
    } while (opt != 0);

    return 0;
}
