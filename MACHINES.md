# Máquinas utilizadas na avaliação experimental

Os experimentos foram executados em duas máquinas. As **métricas estruturais**
(acessos a disco, altura, nº de nós, tamanho do arquivo) são determinísticas e
saíram **idênticas** nas duas (ver `tables/comparison.md` → veredito PASS); apenas
os **tempos** variam, refletindo o hardware.

## Mapeamento dos dados

| Papel na comparação | Máquina | Onde estão os dados |
| --- | --- | --- |
| **A** (`_baseline`) | Notebook (este laptop) | `_baseline/results_*/data.csv` |
| **B** (`.`) | Titan Server (USP) | `results_*/data.csv` (raiz) |

> Em `tables/comparison.md`, a razão **B/A** é, portanto, **Titan ÷ Notebook**
> (>1 significa Titan mais lento que o notebook neste workload).

---

## Máquina A — Notebook (este laptop)

| Item | Valor |
| --- | --- |
| Função | Execução de referência (baseline) |
| CPU | Intel® Core™ 7 240H |
| Núcleos / threads | 8 núcleos físicos / 16 threads (2 por núcleo) |
| Cache | L1d 384 KiB · L1i 256 KiB · L2 10 MiB · L3 24 MiB |
| Memória RAM | 16 GiB (≈15 GiB visíveis) |
| Sistema operacional | Ubuntu 24.04.3 LTS sob **WSL2** (Windows) |
| Kernel | Linux 6.6.87.2-microsoft-standard-WSL2 (x86_64) |
| Compilador | g++ (Ubuntu 13.3.0) — C++17, flags `-O2 -Wall -Wextra` |
| Build / ferramentas | GNU Make 4.3 · Graphviz 2.43.0 · Python 3.12.3 |
| Armazenamento do `.bin` | ext4 (disco virtual do WSL2) |

**Observação (WSL2):** o arquivo binário fica no sistema de arquivos ext4
virtualizado do WSL2. Como `writeNode` faz `flush()` para o cache de páginas do
SO (não `fsync`), o I/O lógico medido pelo contador `disk_accesses` é o mesmo de
qualquer máquina; o tempo de parede, porém, sofre influência da camada de
virtualização do WSL2.

---

## Máquina B — Titan Server (USP)

> **A PREENCHER** — coletar na própria Titan com:
> ```bash
> lscpu | grep -E "Model name|^CPU\(s\)|Thread|Core|Socket|cache"
> free -h
> . /etc/os-release; echo "$PRETTY_NAME"; uname -srmo
> g++ --version | head -1; make --version | head -1; python3 --version
> ```

| Item | Valor |
| --- | --- |
| Função | Execução comparativa |
| CPU | _a preencher_ |
| Núcleos / threads | _a preencher_ |
| Cache | _a preencher_ |
| Memória RAM | _a preencher_ |
| Sistema operacional | _a preencher_ |
| Kernel | _a preencher_ |
| Compilador | _a preencher_ |
| Build / ferramentas | _a preencher_ |
| Armazenamento do `.bin` | _a preencher_ |

---

## Resumo da comparação de tempo (Titan ÷ Notebook)

Extraído de `tables/comparison.md` (mediana por experimento):

| Experimento | razão B/A mediana | mín | máx |
| --- | --- | --- | --- |
| `results_order_m_impact` | 1.20× | 0.32× | 2.17× |
| `results_set_size_scaling` | 1.16× | 0.28× | 2.28× |
| `results_node_reuse` | 1.18× | 1.05× | 1.69× |

Interpretação: no conjunto, a Titan ficou ~15–20% **mais lenta** que o notebook
em tempo de parede mediano (com casos indo de ~0.3× a ~2.3×), enquanto **todas**
as métricas de acesso a disco foram idênticas — confirmando que a diferença é
puramente de hardware/ambiente, não de comportamento do algoritmo.
