#!/usr/bin/env bash
# =============================================================================
# run_all.sh — Orquestrador da avaliação experimental do trabalho de Árvore B.
#
# Recompila o driver bench/bench para cada ordem m (M é constante de compilação)
# e varre a matriz de experimentos, gravando um CSV por experimento dentro da
# respectiva pasta results_<descricao>/. Também gera os arquivos .dot/.png dos
# grafos (início e fim do processo) em results_graphs/.
#
# Uso:
#   bash experiments/run_all.sh            # matriz completa (inclui 10^6)
#   LIGHT=1 bash experiments/run_all.sh    # pula os testes de 10^6 (mais rápido)
#
# Idempotente: limpa os CSVs/grafos antes de cada execução.
# =============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BENCH="$ROOT/bench/bench"
TMP="$ROOT/_bench_tmp.bin"

# Pastas de resultado (uma por experimento) + descrições.
D_ORDER="$ROOT/results_order_m_impact"
D_SCALE="$ROOT/results_set_size_scaling"
D_REUSE="$ROOT/results_node_reuse"
D_GRAPH="$ROOT/results_graphs"
mkdir -p "$D_ORDER" "$D_SCALE" "$D_REUSE" "$D_GRAPH"

CSV_ORDER="$D_ORDER/data.csv"
CSV_SCALE="$D_SCALE/data.csv"
CSV_REUSE="$D_REUSE/data.csv"
rm -f "$CSV_ORDER" "$CSV_SCALE" "$CSV_REUSE"
rm -f "$D_GRAPH"/*.dot "$D_GRAPH"/*.png 2>/dev/null || true

# --- cache de build: só recompila quando a ordem m muda -----------------------
BUILT_M=""
ensure_build() {
  local m="$1"
  if [[ "$m" != "$BUILT_M" ]]; then
    echo ">>> compilando bench para m=$m" >&2
    make bench M="$m" >/dev/null 2>&1
    BUILT_M="$m"
  fi
}

run() {  # run <m> <args...>
  local m="$1"; shift
  ensure_build "$m"
  "$BENCH" --m "$m" --file "$TMP" "$@"
}

echo "############ EXPERIMENTO 1: impacto da ordem m ############"
# N fixo = 10^5; varia m do pequeno (3) ao grande (1000, = 10^3 do enunciado).
# Mostra a queda de altura e de I/O médio por operação à medida que m cresce.
ORDER_MS="3 4 5 8 16 32 64 100 128 256 512 1000"
for m in $ORDER_MS; do
  for mode in rand seq; do
    run "$m" --op full --N 100000 --mode "$mode" --reuse 1 \
        --label "order_impact" --csv "$CSV_ORDER"
  done
done

echo "############ EXPERIMENTO 2: escala do conjunto ############"
# Ordens pequena (3) e grandes (100=10^2, 1000=10^3); N = 10^3..10^6,
# aleatório e sequencial. Captura I/O médio, altura e tempo CPU vs espera I/O.
SCALE_MS="3 100 1000"
if [[ "${LIGHT:-0}" == "1" ]]; then
  SCALE_NS="1000 10000 100000"
else
  SCALE_NS="1000 10000 100000 1000000"
fi
for m in $SCALE_MS; do
  for N in $SCALE_NS; do
    for mode in rand seq; do
      run "$m" --op full --N "$N" --mode "$mode" --reuse 1 \
          --label "set_size" --csv "$CSV_SCALE"
    done
  done
done

echo "############ EXPERIMENTO 3: ocupação do arquivo / reaproveitamento ############"
# Workload de churn: insere N, remove metade, reinsere metade de chaves novas.
# Compara o tamanho final do arquivo COM (reuse=1) e SEM (reuse=0) free list.
REUSE_MS="3 5 100"
REUSE_NS="10000 100000"
for m in $REUSE_MS; do
  for N in $REUSE_NS; do
    for r in 1 0; do
      run "$m" --op churn --N "$N" --mode rand --reuse "$r" \
          --label "node_reuse" --csv "$CSV_REUSE"
    done
  done
done

echo "############ GRAFOS: imagens do início e do fim do processo ############"
# Árvores pequenas e legíveis. Mesmo conjunto de chaves (seed fixo) para m=3,5,10
# evidencia visualmente a redução de altura conforme m cresce.
# Para m=3 também guardamos um snapshot do "início" (após 6 inserções).
GRAPH_N=40
for m in 3 5 10; do
  if [[ "$m" == "3" ]]; then
    run "$m" --op insert --N "$GRAPH_N" --mode rand --reuse 1 --seed 42 \
        --dot-prefix "$D_GRAPH/m${m}" --dot-at 6 >/dev/null
  else
    run "$m" --op insert --N "$GRAPH_N" --mode rand --reuse 1 --seed 42 \
        --dot-prefix "$D_GRAPH/m${m}" >/dev/null
  fi
done

echo ">>> renderizando PNGs dos grafos" >&2
for dot in "$D_GRAPH"/*.dot; do
  [[ -e "$dot" ]] || continue
  png="${dot%.dot}.png"
  dot -Tpng -Gdpi=110 "$dot" -o "$png" 2>/dev/null && echo "    $png" >&2
done

rm -f "$TMP"
echo "############ CONCLUÍDO ############"
echo "CSVs:"
echo "  $CSV_ORDER"
echo "  $CSV_SCALE"
echo "  $CSV_REUSE"
echo "Grafos: $D_GRAPH"
