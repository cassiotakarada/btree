# Avaliação Experimental — Árvore B em Memória Secundária

Este documento descreve a bateria de experimentos exigida pelo enunciado e como
reproduzi-la. Toda interação com o arquivo continua sendo **um nó por vez** (a
classe `DiskManager` lê/escreve registros de `PAGE_SIZE`); o benchmark apenas
orquestra as operações e coleta métricas — nada da árvore é carregado em massa
para a memória principal.

## Como reproduzir

```bash
# matriz completa (inclui 10^6 chaves — leva alguns minutos)
bash experiments/run_all.sh
python3 experiments/make_tables.py

# versão rápida (pula 10^6)
LIGHT=1 bash experiments/run_all.sh
python3 experiments/make_tables.py
```

O driver de benchmark é `bench/bench.cpp`, compilado por ordem *m* via
`make bench M=<m>` (M é constante de compilação, como no programa principal).

## Itens do enunciado → onde estão

| Item do enunciado | Experimento | Pasta de resultados | Tabelas |
| --- | --- | --- | --- |
| **Ordem m** (pequeno=3, grande=10²/10³) — ponto de redução de I/O | Exp. 1 | `results_order_m_impact/` | `tables/exp1_*` |
| **Tamanho do conjunto** (10³–10⁶, aleatório e sequencial) | Exp. 2 | `results_set_size_scaling/` | `tables/exp2_*` |
| **Número de acessos ao disco** — média de I/O por operação | Exp. 1 e 2 | (coluna `avg_io_per_op`) | `tables/exp1_*`, `tables/exp2_*` |
| **Ocupação do arquivo** — com e sem reaproveitamento de nós | Exp. 3 | `results_node_reuse/` | `tables/exp3_*` |
| **Tempo de execução** — CPU vs espera de I/O | Exp. 4 | `results_cpu_vs_io/` | `tables/exp4_*` |
| **Visualização dos grafos** (início e fim) | — | `results_graphs/` | (imagens PNG) |

Cada pasta `results_*` tem um `description.md` explicando a configuração e as
métricas daquele experimento.

## Métricas coletadas (colunas dos CSVs)

- `disk_accesses`, `avg_io_per_op` — leituras+escritas de nós (o I/O lógico que o
  enunciado pede), total e por operação.
- `wall_ms`, `cpu_user_ms`, `cpu_sys_ms`, `io_wait_ms` — decomposição de tempo via
  `getrusage` + relógio de parede.
- `height` — altura da árvore.
- `total_nodes`, `free_nodes`, `file_bytes` — ocupação do arquivo binário.

## Principais conclusões (dados aleatórios, N=100k)

- **A altura cai e o I/O por operação despenca** conforme *m* cresce: de altura 14
  e ~13 I/O por busca (m=3) para altura 2 e 2 I/O por busca (m≥512). O ganho
  **satura** a partir de m≈64–128 (a árvore já tem 2–3 níveis); além disso o custo
  migra do disco para a CPU (busca linear dentro do nó).
- **Inserção sequencial** gera arquivos maiores que a aleatória (nós ~50% cheios
  vs ~69%).
- **O reaproveitamento de nós economiza ~27–30%** do tamanho do arquivo no
  workload de churn.
- Com `flush()` indo ao **cache de páginas do SO**, a maior parte do tempo de I/O
  aparece como **CPU de sistema**; a métrica honesta e independente de cache é o
  contador `disk_accesses`.
