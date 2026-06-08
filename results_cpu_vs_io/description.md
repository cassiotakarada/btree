# Experimento 4 — Tempo de CPU vs espera de I/O

## Objetivo
Diferenciar, no tempo total de execução, a parcela gasta em **CPU** da parcela
de **espera de I/O**, conforme pedido no enunciado.

## Origem dos dados
Esta pasta é uma **visão** (extrato) do Experimento 2 (escala do conjunto), modo
aleatório. O driver coleta, em cada fase, via `getrusage(RUSAGE_SELF)`:
- `cpu_user_ms` — tempo de CPU em espaço de usuário (lógica da árvore: busca
  dentro do nó, deslocamentos de vetores, splits/merges).
- `cpu_sys_ms` — tempo de CPU em espaço de kernel (syscalls `read`/`write`/`lseek`
  dos nós — o custo de I/O que de fato passa pela CPU).
- `io_wait_ms = wall − (user + sys)` — tempo de parede não atribuível à CPU
  (espera por disco/escalonamento).

## Ressalva metodológica
A implementação grava cada nó com `fstream::flush()`, que envia os dados ao
**cache de páginas do SO** (não força `fsync` ao disco físico). Logo, em uma
máquina com RAM suficiente, a maior parte do "I/O" aparece como **CPU de sistema**
(`sys`), e a espera de I/O pura (`io_wait`) tende a ser pequena. Isso é esperado e
está documentado: o número honesto de acessos lógicos a disco é o
`disk_accesses` (contador de nós lidos/escritos), independente do cache do SO.

## Métricas (colunas de `data.csv`)
`m, N, modo, fase, wall_ms, cpu_user_ms, cpu_sys_ms, io_wait_ms, %cpu, %io`.

Tabela derivada: `tables/exp4_cpu_vs_io.*`.
