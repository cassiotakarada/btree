# Experimento 1 — Impacto da ordem *m*

## Objetivo
Observar como a **ordem m** da Árvore B influencia a **altura** e o **número médio
de acessos ao disco (I/O)** por operação, identificando o ponto a partir do qual
aumentar *m* deixa de reduzir significativamente o I/O.

## Configuração
- **N fixo = 100.000** chaves.
- **Ordens testadas:** m ∈ {3, 4, 5, 8, 16, 32, 64, 100, 128, 256, 512, 1000}.
  - Cobre o intervalo do enunciado: m pequeno (=3) e m grande (=10² e 10³).
- **Modos de inserção:** aleatório (`rand`) e sequencial (`seq`).
- **Operações por rodada:** inserção de N chaves, busca das N chaves, remoção das N chaves.
- **Reaproveitamento de nós:** ligado.

## Métricas (colunas de `data.csv`)
- `disk_accesses` / `avg_io_per_op` — total e média de leituras+escritas de nós por operação.
- `wall_ms`, `cpu_user_ms`, `cpu_sys_ms`, `io_wait_ms` — tempo de parede, CPU de usuário, CPU de sistema e espera de I/O estimada (`wall − cpu`).
- `height` — altura da árvore (níveis).
- `total_nodes`, `free_nodes`, `file_bytes` — ocupação do arquivo binário.

## O que observar
À medida que *m* cresce, a altura cai (≈ log_m N) e o I/O médio por busca despenca;
acima de certo *m* o ganho satura porque a árvore já tem pouquíssimos níveis e o
custo passa a ser dominado pela busca linear dentro do nó (em CPU, não em I/O).

Tabelas e gráficos derivados: `tables/exp1_*`.
