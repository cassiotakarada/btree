#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <random>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include "src/disk_manager.hpp"
#include "src/btree.hpp"

// RAII: começa a contar no construtor e, no destrutor, imprime o tempo
// elapsed e soma no acumulador global do programa. Use criando uma instância
// local no início do bloco da operação que você quer medir.
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
              << "0. Voltar ao menu principal\n"
              << "> ";
}

// Insere chaves aleatórias até a árvore atingir a altura mínima desejada.
// Útil para preparar o arquivo btree.bin antes de testar mSearch interativamente.
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

// Roda 3 fases (insert, mSearch, delete) em uma árvore separada (_exp.bin),
// cronometrando cada uma e somando os acessos a disco. Retorna o tempo total
// (ms) das 3 fases — útil para alimentar o acumulador global.
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

// Submenu "Outras opções": agrupa ações utilitárias / pesadas que não são parte
// das operações principais (CRUD + métricas). Roda em loop próprio até o usuário
// escolher 0 (voltar).
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
            // Sempre gera tree.dot (sobrepõe) e renderiza tree.png em seguida.
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
        }
    } while (opt != 0);
}

int main(int argc, char* argv[]) {
    std::string path = (argc > 1) ? argv[1] : "btree.bin";
    DiskManager dm(path);
    BTree bt(dm);

    // Acumula o tempo de execução de todas as operações já feitas na sessão.
    // Reset junto com o contador de acessos via "Outras opções → Zerar contador".
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
