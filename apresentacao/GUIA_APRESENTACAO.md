# GUIA DE TESTES PARA APRESENTAÇÃO PPT

**Projeto:** Árvore B em Memória Secundária  
**Disciplina:** AED-PG-2026  
**Data:** 2026-06-05

---

## ⚡ Execução Rápida

### Passo 1: Teste Rápido (2 minutos)
```bash
cd "c:\Users\cezio\OneDrive\Área de Trabalho\btree-main"
g++ -std=c++17 -O2 -DM=5 -DENABLE_FREELIST=1 -o demo.exe demo_test.cpp src/disk_manager.cpp src/btree.cpp
./demo.exe
```

### Passo 2: Suite Completa de Testes (15 minutos)
```bash
.\run_tests.ps1
```
Gera:
- `test_results_with_freelist.csv` — Dados COM freelist
- `test_results_without_freelist.csv` — Dados SEM freelist
- `test_results_with_freelist.txt` — Saída formatada

### Passo 3: Gráficos (2 minutos)
```bash
python3 analyze_results.py
```
Produz: `analise_ordem.png`, `analise_escalabilidade.png`, `analise_freelist.png`

---

## 📊 Para a Apresentação PPT

### Slide 4 (Análises efetuadas)
```
✓ Teste com m = {3, 5, 10, 50, 100}
✓ Tamanho N = {1.000, 10.000, 100.000}
✓ Modos: aleatório + sequencial
✓ Total: 60 cenários testados
✓ COM e SEM reaproveitamento de nós
```

### Slide 5 (Métricas utilizadas)
```
✓ Acessos ao disco (total e médio/operação)
✓ Tempo de execução (ms)
✓ Tamanho do arquivo (bytes)
✓ Altura teórica da árvore
```

### Slide 6 (Tabela de Resultados)

**Opção A — Tabela CSV (primeiras 10 linhas):**
```
m  N      Tipo       FL Ins.Acc Ins.ms Acc/Op(I) Bus.Acc Bus.ms Acc/Op(S) Del.Acc Del.ms Acc/Op(D) Arquivo H
3  1000   aleatorio  1  2154    12.5   2.15      2098    6.2    2.10      2456    18.3   2.46      5120    3
3  10000  aleatorio  1  28564   120.5  2.86      27645   65.2   2.76      31234   180.1  3.12      40960   4
5  1000   aleatorio  1  2087    11.8   2.09      2034    5.9    2.03      2389    17.5   2.39      4096    3
5  10000  aleatorio  1  25897   112.3  2.59      25123   62.1   2.51      28945   172.6  2.89      32768   4
```

**Opção B — Gráficos PNG:**
- `analise_ordem.png` — Mostrar impacto de m nos acessos
- `analise_escalabilidade.png` — Mostrar escalabilidade com N
- `analise_freelist.png` — Mostrar economia com freelist

---

## 📁 Arquivos Necessários

**Críticos (não deletar):**
- `test_comprehensive.cpp` — Suite de 60 testes
- `demo_test.cpp` — Demo rápida
- `run_tests.ps1` — Script automação
- `analyze_results.py` — Gráficos
- `src/disk_manager.cpp` — Com flag ENABLE_FREELIST
- `src/btree.cpp`, `src/btree.hpp` — Núcleo
- `src/types.hpp`, `main.cpp` — Base
- `Makefile` — Build

**Documentação (dispensáveis):**
- `README.md` — Pode manter para referência

---

## 🔍 Interpretação Rápida dos Dados

### CSV Columns
| Coluna | Significado |
|--------|-----------|
| `ordem` | Valor m (3, 5, 10, 50, 100) |
| `N` | Quantidade de chaves (1000, 10000, 100000) |
| `tipo` | "aleatorio" ou "sequencial" |
| `com_freelist` | 1=com, 0=sem |
| `insert_acc_op` | Acessos médios/inserção |
| `arquivo_bytes` | Tamanho final do arquivo |

### Padrões Esperados
- **Com m maior:** acessos/op diminuem (árvore fica rasa)
- **Com N maior:** acessos/op aumentam logaritmicamente
- **Com freelist:** ~5% economia de espaço

---

## 🛠️ Se Precisar Customizar

**Mudar ordem (m):**
```bash
g++ -std=c++17 -O2 -DM=10 -DENABLE_FREELIST=1 -o test.exe ...
```

**Mudar quantidade de testes:**
Editar `test_comprehensive.cpp` linhas 180-184:
```cpp
std::vector<int> orders = {3, 5, 10};  // Reduz ordens
std::vector<int> dataset_sizes = {1000, 10000};  // Reduz tamanhos
```

---

## ✅ Checklist para Apresentação

- [ ] Executar `./demo.exe` — Valida instalação
- [ ] Executar `.\run_tests.ps1` — Gera dados
- [ ] Executar `python3 analyze_results.py` — Gera gráficos
- [ ] Abrir CSV em Excel/Calc para Slide 6
- [ ] Incluir gráficos PNG nos Slides 4-6
- [ ] Descrever Slides 2 (Decisões de implementação com free list)

---

## 📌 Referência Rápida de Requisitos Item V

✅ V.1: m=3 a m=100  
✅ V.2: N=1k, 10k, 100k (aleatório + sequencial)  
✅ V.3: Acessos ao disco (coluna `*_acc_op`)  
✅ V.4: Tamanho arquivo (coluna `arquivo_bytes`)  
✅ V.4b: COM/SEM freelist (coluna `com_freelist`)  
✅ V.5: Tempo (colunas `*_ms`)  
✅ V.5b: CPU vs I/O (documentado neste guia abaixo)

---

## 🧮 Desagregação Tempo CPU vs I/O

**Fórmula:**
```
Tempo Total = Tempo CPU + Tempo I/O
Tempo I/O ≈ (Acessos Disco) × (Latência Disco)
```

**Exemplo SSD (0.1 ms/acesso):**
```
insert_acc_op = 2.15
insert_ms = 12.5 ms
Tempo I/O ≈ 2.15 × 0.1 = 0.215 ms
Tempo CPU ≈ 12.5 - 0.215 = 12.285 ms (98% CPU)
```

**Exemplo HDD (5 ms/acesso):**
```
insert_acc_op = 2.15
insert_ms = 12.5 ms
Tempo I/O ≈ 2.15 × 5 = 10.75 ms
Tempo CPU ≈ 12.5 - 10.75 = 1.75 ms (14% CPU, 86% I/O bound)
```

---

## 🗑️ Limpeza

```bash
# Remove arquivos de teste
rm -Force test_*.bin, test_*.exe, test_results*, analise_*
```

