# Comparação de resultados: _baseline (A) × . (B)
Métricas estruturais devem ser **idênticas** (determinísticas); tempos variam com o hardware.

## results_order_m_impact
- linhas em comum: 72 | só em A: 0 | só em B: 0

✅ métricas determinísticas **idênticas** em todas as linhas em comum.

## results_set_size_scaling
- linhas em comum: 72 | só em A: 0 | só em B: 0

✅ métricas determinísticas **idênticas** em todas as linhas em comum.

## results_node_reuse
- linhas em comum: 36 | só em A: 0 | só em B: 0

✅ métricas determinísticas **idênticas** em todas as linhas em comum.

## Tempo (wall) — razão B/A (>1 = B mais lento)

| dataset | nº linhas | razão B/A mediana | mín | máx |
| --- | --- | --- | --- | --- |
| results_order_m_impact | 72 | 1.20× | 0.32× | 2.17× |
| results_set_size_scaling | 72 | 1.16× | 0.28× | 2.28× |
| results_node_reuse | 12 | 1.18× | 1.05× | 1.69× |

## Veredito: **PASS — implementação portável e determinística**

(Comparados 1080 valores estruturais; 0 divergências.)
