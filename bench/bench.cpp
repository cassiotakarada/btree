// =============================================================================
// bench.cpp — driver NÃO interativo para a avaliação experimental do trabalho.
//
// A ordem m da árvore é a constante de compilação M (igual ao programa
// principal). Por isso o binário é recompilado para cada m (ver Makefile alvo
// `bench` + script experiments/run_all.sh). Tudo o mais (N, modo, reuso, fase)
// é parâmetro de linha de comando, para o orquestrador varrer a matriz de
// experimentos sem intervenção humana.
//
// Métricas coletadas por fase:
//   - acessos ao disco (readNode + writeNode) e média por operação
//   - tempo de parede (wall), tempo de CPU de usuário e de sistema (getrusage)
//   - "espera de I/O" estimada = wall - (user + sys)
//   - altura da árvore, nós físicos no arquivo, nós livres, tamanho do arquivo
//
// Saída: uma linha CSV por fase em --csv (append). Cabeçalho escrito se o
// arquivo ainda não existir.
// =============================================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sys/resource.h>

#include "../src/disk_manager.hpp"
#include "../src/btree.hpp"

namespace {

struct Args {
    std::string op    = "full";   // insert | search | delete | full | churn
    long        N     = 1000;
    std::string mode  = "rand";   // rand | seq
    bool        reuse = true;
    unsigned    seed  = 42;
    std::string file  = "_bench.bin";
    std::string csv   = "";       // se vazio, imprime no stdout
    std::string label = "";       // descrição do experimento
    int         mLabel = M;       // ordem m (apenas rótulo; M é compile-time)
    std::string dotPrefix = "";   // se não vazio, exporta .dot nos checkpoints
    std::vector<long> dotAt;      // contagens de inserção para snapshot
};

// Tempo de CPU acumulado pelo processo (segundos).
struct CpuTimes { double user; double sys; };
CpuTimes cpuNow() {
    rusage ru{};
    getrusage(RUSAGE_SELF, &ru);
    double u = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1e6;
    double s = ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1e6;
    return {u, s};
}

double parseBool(const std::string& v) { return (v == "1" || v == "true"); }

Args parseArgs(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::cerr << "faltou valor para " << k << "\n"; std::exit(2); }
            return argv[++i];
        };
        if      (k == "--op")        a.op    = next();
        else if (k == "--N")         a.N     = std::stol(next());
        else if (k == "--mode")      a.mode  = next();
        else if (k == "--reuse")     a.reuse = parseBool(next());
        else if (k == "--seed")      a.seed  = static_cast<unsigned>(std::stoul(next()));
        else if (k == "--file")      a.file  = next();
        else if (k == "--csv")       a.csv   = next();
        else if (k == "--label")     a.label = next();
        else if (k == "--m")         a.mLabel = std::stoi(next());
        else if (k == "--dot-prefix")a.dotPrefix = next();
        else if (k == "--dot-at") {
            std::stringstream ss(next());
            std::string tok;
            while (std::getline(ss, tok, ',')) if (!tok.empty()) a.dotAt.push_back(std::stol(tok));
        } else {
            std::cerr << "argumento desconhecido: " << k << "\n";
            std::exit(2);
        }
    }
    return a;
}

// Gera o conjunto de chaves de acordo com o modo.
std::vector<int> makeKeys(long N, const std::string& mode, unsigned seed) {
    std::vector<int> keys(static_cast<size_t>(N));
    if (mode == "seq") {
        for (long i = 0; i < N; ++i) keys[static_cast<size_t>(i)] = static_cast<int>(i + 1);
    } else {
        // Permutação aleatória de 1..N: garante N chaves DISTINTAS (a árvore B
        // ignora duplicatas, então sortear com repetição reduziria o tamanho
        // efetivo do conjunto e distorceria as métricas).
        for (long i = 0; i < N; ++i) keys[static_cast<size_t>(i)] = static_cast<int>(i + 1);
        std::mt19937 rng(seed);
        std::shuffle(keys.begin(), keys.end(), rng);
    }
    return keys;
}

struct Row {
    std::string label; int m; long N; std::string mode; bool reuse; std::string phase;
    long   accesses = 0;
    double wall_ms = 0, user_ms = 0, sys_ms = 0;
    int    height = 0, totalNodes = 0, freeNodes = 0;
    long long fileBytes = 0;
};

void writeHeaderIfNeeded(const std::string& csv) {
    std::ifstream test(csv);
    bool exists = test.good() && test.peek() != std::ifstream::traits_type::eof();
    test.close();
    if (exists) return;
    std::ofstream os(csv, std::ios::app);
    os << "label,m,N,mode,reuse,phase,disk_accesses,avg_io_per_op,"
          "wall_ms,cpu_user_ms,cpu_sys_ms,io_wait_ms,"
          "height,total_nodes,free_nodes,file_bytes\n";
}

void emit(const Args& a, const Row& r) {
    double avg = (a.N > 0) ? static_cast<double>(r.accesses) / static_cast<double>(a.N) : 0.0;
    double ioWait = r.wall_ms - (r.user_ms + r.sys_ms);
    if (ioWait < 0) ioWait = 0;  // ruído de relógio
    std::ostringstream line;
    line.setf(std::ios::fixed); line.precision(4);
    line << r.label << ',' << r.m << ',' << r.N << ',' << r.mode << ','
         << (r.reuse ? 1 : 0) << ',' << r.phase << ','
         << r.accesses << ',' << avg << ','
         << r.wall_ms << ',' << r.user_ms << ',' << r.sys_ms << ',' << ioWait << ','
         << r.height << ',' << r.totalNodes << ',' << r.freeNodes << ',' << r.fileBytes;
    if (a.csv.empty()) {
        std::cout << line.str() << "\n";
    } else {
        std::ofstream os(a.csv, std::ios::app);
        os << line.str() << "\n";
    }
    // Eco curto no stderr para acompanhamento ao vivo.
    std::cerr << "[bench] m=" << r.m << " N=" << r.N << " " << r.mode
              << " reuse=" << r.reuse << " " << r.phase
              << " : io=" << r.accesses << " (" << avg << "/op) wall="
              << r.wall_ms << "ms h=" << r.height
              << " nodes=" << r.totalNodes << " free=" << r.freeNodes
              << " size=" << r.fileBytes << "B\n";
}

} // namespace

int main(int argc, char** argv) {
    Args a = parseArgs(argc, argv);
    if (!a.csv.empty()) writeHeaderIfNeeded(a.csv);

    std::remove(a.file.c_str());  // sempre começa com arquivo limpo

    DiskManager dm(a.file);
    dm.setReuse(a.reuse);
    BTree bt(dm);

    std::vector<int> keys = makeKeys(a.N, a.mode, a.seed);
    std::vector<long> dotAt = a.dotAt;
    std::sort(dotAt.begin(), dotAt.end());
    size_t dotIdx = 0;

    using clock = std::chrono::high_resolution_clock;

    auto fillRow = [&](Row& r) {
        r.label = a.label; r.m = a.mLabel; r.N = a.N; r.mode = a.mode; r.reuse = a.reuse;
        r.height = bt.height();
        r.totalNodes = dm.totalNodes();
        r.freeNodes = dm.freeNodes();
        r.fileBytes = static_cast<long long>(dm.fileSizeBytes());
    };

    // ---------------- INSERÇÃO ----------------
    if (a.op == "insert" || a.op == "full") {
        dm.resetCounter();
        auto c0 = cpuNow(); auto t0 = clock::now();
        long done = 0;
        for (int k : keys) {
            bt.insertB(k);
            ++done;
            if (!a.dotPrefix.empty() && dotIdx < dotAt.size() && done == dotAt[dotIdx]) {
                bt.exportDot(a.dotPrefix + "_ins" + std::to_string(done) + ".dot");
                ++dotIdx;
            }
        }
        auto t1 = clock::now(); auto c1 = cpuNow();
        Row r; r.phase = "insert";
        r.accesses = dm.diskAccesses();
        r.wall_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        r.user_ms = (c1.user - c0.user) * 1000.0;
        r.sys_ms  = (c1.sys  - c0.sys ) * 1000.0;
        fillRow(r);
        emit(a, r);
        if (!a.dotPrefix.empty()) bt.exportDot(a.dotPrefix + "_final.dot");
    }

    // ---------------- BUSCA ----------------
    if (a.op == "search" || a.op == "full") {
        if (a.op == "search") for (int k : keys) bt.insertB(k);  // precisa popular
        dm.resetCounter();
        auto c0 = cpuNow(); auto t0 = clock::now();
        volatile bool sink = false;
        for (int k : keys) { auto [p, i, f] = bt.mSearch(k); (void)p; (void)i; sink ^= f; }
        auto t1 = clock::now(); auto c1 = cpuNow();
        Row r; r.phase = "search";
        r.accesses = dm.diskAccesses();
        r.wall_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        r.user_ms = (c1.user - c0.user) * 1000.0;
        r.sys_ms  = (c1.sys  - c0.sys ) * 1000.0;
        fillRow(r);
        emit(a, r);
    }

    // ---------------- REMOÇÃO ----------------
    if (a.op == "delete" || a.op == "full") {
        if (a.op == "delete") for (int k : keys) bt.insertB(k);  // precisa popular
        std::vector<int> order = keys;
        std::shuffle(order.begin(), order.end(), std::mt19937(a.seed + 7));
        dm.resetCounter();
        auto c0 = cpuNow(); auto t0 = clock::now();
        for (int k : order) bt.deleteB(k);
        auto t1 = clock::now(); auto c1 = cpuNow();
        Row r; r.phase = "delete";
        r.accesses = dm.diskAccesses();
        r.wall_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        r.user_ms = (c1.user - c0.user) * 1000.0;
        r.sys_ms  = (c1.sys  - c0.sys ) * 1000.0;
        fillRow(r);
        emit(a, r);
    }

    // ---------------- CHURN (ocupação do arquivo / reaproveitamento) --------
    // build N -> delete metade -> reinsere metade de chaves novas.
    // Com reuso ligado, a reinserção consome a free list (arquivo não cresce).
    // Com reuso desligado, allocNode acrescenta sempre ao fim (arquivo incha).
    if (a.op == "churn") {
        // build
        for (int k : keys) bt.insertB(k);
        { Row r; r.phase = "churn_build"; fillRow(r); r.accesses = 0; emit(a, r); }

        // delete metade (índices pares)
        long half = a.N / 2;
        for (long i = 0; i < a.N; i += 2) bt.deleteB(keys[static_cast<size_t>(i)]);
        { Row r; r.phase = "churn_after_delete"; fillRow(r); r.accesses = 0; emit(a, r); }

        // reinsere `half` chaves novas (valores > N, garantidamente inéditos)
        dm.resetCounter();
        auto c0 = cpuNow(); auto t0 = clock::now();
        for (long i = 0; i < half; ++i) bt.insertB(static_cast<int>(a.N + 1 + i));
        auto t1 = clock::now(); auto c1 = cpuNow();
        Row r; r.phase = "churn_refill";
        r.accesses = dm.diskAccesses();
        r.wall_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        r.user_ms = (c1.user - c0.user) * 1000.0;
        r.sys_ms  = (c1.sys  - c0.sys ) * 1000.0;
        fillRow(r);
        emit(a, r);
    }

    std::remove(a.file.c_str());  // limpa o arquivo temporário
    return 0;
}
