# Experimento 3 — Ocupação do arquivo: com vs sem reaproveitamento de nós

## Objetivo
Medir o **tamanho final do arquivo binário** com e sem o mecanismo de
reaproveitamento de nós (free list), demonstrando o impacto da otimização.

## Como o reaproveitamento funciona
- Ao remover um nó da árvore, `DiskManager::freeNode` o marca como livre
  (`n = -1`) e o encadeia numa **lista de nós livres** cuja cabeça fica no header
  (`free_head`), usando `K[1]` de cada nó livre como ponteiro para o próximo.
- `DiskManager::allocNode` reutiliza primeiro a cabeça da free list; só acrescenta
  um registro novo ao fim do arquivo quando a lista está vazia.
- O toggle `DiskManager::setReuse(false)` desliga esse reuso: `allocNode` passa a
  **sempre** acrescentar ao fim — modelando "sem reaproveitamento".

## Configuração — workload de *churn*
Para cada (m, N): **insere N** chaves → **remove metade** → **reinsere metade**
de chaves novas (valores inéditos). Esse vai-e-vem é o que cria nós livres e
exercita (ou não) a free list.
- **Ordens:** m ∈ {3, 5, 100}.
- **N ∈ {10.000, 100.000}**, modo aleatório.
- **reuse ∈ {1 (com), 0 (sem)}**.

## Métricas
- `total_nodes` — registros físicos no arquivo (maior RRN alocado).
- `free_nodes` — nós atualmente na free list.
- `file_bytes` — tamanho final do arquivo.
- A fase relevante é `churn_refill` (estado final). As fases `churn_build` e
  `churn_after_delete` registram o caminho até lá.

## O que observar
Com reuso, a reinserção consome os nós livres e o arquivo **não cresce**; sem
reuso, cada alocação acrescenta ao fim e o arquivo **incha** proporcionalmente ao
número de nós que ficaram livres.

Tabela e gráfico derivados: `tables/exp3_node_reuse.*`.
