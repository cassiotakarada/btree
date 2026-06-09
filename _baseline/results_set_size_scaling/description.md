# Experimento 2 — Escala do conjunto (10³ a 10⁶ chaves)

## Objetivo
Medir como o desempenho (I/O médio, altura e tempo) escala com o **tamanho do
conjunto N**, comparando uma ordem pequena com ordens grandes e inserção
aleatória vs. sequencial.

## Configuração
- **N ∈ {1.000, 10.000, 100.000, 1.000.000}** (= 10³, 10⁴, 10⁵, 10⁶).
- **Ordens:** m ∈ {3, 100, 1000} — pequena (3) e grandes (10², 10³ do enunciado).
- **Modos:** aleatório (`rand`) e sequencial (`seq`).
- **Operações:** inserção, busca e remoção de todas as N chaves.
- **Reaproveitamento de nós:** ligado.

> Observação: chaves geradas como permutação de 1..N (aleatório) ou 1..N em ordem
> (sequencial), garantindo N chaves **distintas** (a Árvore B ignora duplicatas).

## Métricas
Mesmas colunas do Experimento 1 (ver `results_order_m_impact/description.md`).
O foco aqui é a **tendência logarítmica** do I/O por operação: como a Árvore B é
balanceada, o I/O por busca cresce com log_m N — praticamente constante em ordens
grandes mesmo multiplicando N por 1000.

## O que observar
- A inserção **sequencial** produz nós ~50% cheios (split sempre à direita),
  gerando árvores maiores/arquivos maiores que a inserção **aleatória** (~69%).
- O tempo total cresce de forma quase linear com N; o I/O **por operação** quase não muda.

Tabelas e gráficos derivados: `tables/exp2_*`. A análise CPU vs I/O usa estes
mesmos dados (ver `results_cpu_vs_io/`).
