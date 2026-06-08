# Experimento 4 — Tempo de CPU vs espera de I/O (aleatório)

`cpu = user + sys` (sys captura as syscalls de leitura/escrita de nós); `io_wait = wall - cpu`.

> Em fases curtas (N pequeno) a granularidade do relógio de CPU (`getrusage`) é da ordem do próprio tempo medido, então `%cpu` é limitado a 100%. As linhas confiáveis para a razão CPU×I/O são as de **N = 10⁶**, onde fica claro que ~98% do tempo é CPU (predominância da CPU de sistema, pois o `flush` vai ao cache de páginas do SO, não ao disco físico).

| m | N | modo | fase | wall_ms | cpu_user_ms | cpu_sys_ms | io_wait_ms | %cpu | %io |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 3 | 1,000 | rand | delete | 23.70 | 4.27 | 20.39 | 0 | 100 | 0 |
| 3 | 1,000 | rand | insert | 25.21 | 8.31 | 17.94 | 0 | 100 | 0 |
| 3 | 1,000 | rand | search | 5.02 | 3.13 | 2.09 | 0 | 100 | 0 |
| 3 | 10,000 | rand | delete | 190 | 62.33 | 137 | 0 | 100 | 0 |
| 3 | 10,000 | rand | insert | 160 | 57.54 | 109 | 0 | 100 | 0 |
| 3 | 10,000 | rand | search | 58.26 | 3.58 | 57.05 | 0 | 100 | 0 |
| 3 | 100,000 | rand | delete | 2,328 | 831 | 1,701 | 0 | 100 | 0 |
| 3 | 100,000 | rand | insert | 1,943 | 745 | 1,375 | 0 | 100 | 0 |
| 3 | 100,000 | rand | search | 1,122 | 440 | 784 | 0 | 100 | 0 |
| 3 | 1,000,000 | rand | delete | 32,743 | 7,429 | 25,259 | 53.83 | 99.80 | 0.200 |
| 3 | 1,000,000 | rand | insert | 28,818 | 6,614 | 21,654 | 550 | 98.10 | 1.90 |
| 3 | 1,000,000 | rand | search | 21,885 | 6,112 | 15,200 | 572 | 97.40 | 2.60 |
| 100 | 1,000 | rand | delete | 7.40 | 3.91 | 3.91 | 0 | 100 | 0 |
| 100 | 1,000 | rand | insert | 5.26 | 3.60 | 1.97 | 0 | 100 | 0 |
| 100 | 1,000 | rand | search | 1.58 | 0.832 | 0.832 | 0 | 100 | 0 |
| 100 | 10,000 | rand | delete | 60.87 | 12.95 | 51.29 | 0 | 100 | 0 |
| 100 | 10,000 | rand | insert | 40.12 | 16.34 | 26.00 | 0 | 100 | 0 |
| 100 | 10,000 | rand | search | 22.50 | 7.82 | 15.93 | 0 | 100 | 0 |
| 100 | 100,000 | rand | delete | 790 | 206 | 627 | 0 | 100 | 0 |
| 100 | 100,000 | rand | insert | 548 | 120 | 458 | 0 | 100 | 0 |
| 100 | 100,000 | rand | search | 246 | 89.07 | 170 | 0 | 100 | 0 |
| 100 | 1,000,000 | rand | delete | 10,191 | 2,571 | 8,232 | 0 | 100 | 0 |
| 100 | 1,000,000 | rand | insert | 7,517 | 2,961 | 5,089 | 0 | 100 | 0 |
| 100 | 1,000,000 | rand | search | 5,143 | 2,006 | 3,589 | 0 | 100 | 0 |
| 1,000 | 1,000 | rand | delete | 25.20 | 13.29 | 13.65 | 0 | 100 | 0 |
| 1,000 | 1,000 | rand | insert | 31.59 | 17.43 | 16.33 | 0 | 100 | 0 |
| 1,000 | 1,000 | rand | search | 11.89 | 2.89 | 9.84 | 0 | 100 | 0 |
| 1,000 | 10,000 | rand | delete | 82.59 | 35.43 | 52.85 | 0 | 100 | 0 |
| 1,000 | 10,000 | rand | insert | 80.89 | 33.91 | 52.56 | 0 | 100 | 0 |
| 1,000 | 10,000 | rand | search | 22.00 | 16.11 | 7.41 | 0 | 100 | 0 |
| 1,000 | 100,000 | rand | delete | 856 | 342 | 573 | 0 | 100 | 0 |
| 1,000 | 100,000 | rand | insert | 744 | 350 | 446 | 0 | 100 | 0 |
| 1,000 | 100,000 | rand | search | 249 | 142 | 125 | 0 | 100 | 0 |
| 1,000 | 1,000,000 | rand | delete | 10,317 | 4,184 | 7,059 | 0 | 100 | 0 |
| 1,000 | 1,000,000 | rand | insert | 11,207 | 2,921 | 6,609 | 1,678 | 85.00 | 15.00 |
| 1,000 | 1,000,000 | rand | search | 4,615 | 2,560 | 2,276 | 0 | 100 | 0 |
