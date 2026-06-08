#!/usr/bin/env python3
# =============================================================================
# compare.py — Compara os resultados de duas máquinas (ou duas execuções).
#
# Métricas DETERMINÍSTICAS (devem ser IDÊNTICAS em qualquer hardware, pois só
# dependem do algoritmo e do conjunto de chaves):
#   disk_accesses, avg_io_per_op, height, total_nodes, free_nodes, file_bytes
# Métricas DEPENDENTES DE HARDWARE (variam; reportamos a razão B/A):
#   wall_ms, cpu_user_ms, cpu_sys_ms
#
# Uso:
#   python3 experiments/compare.py <dir_A> <dir_B>
# onde cada dir contém as pastas results_*/data.csv (ex.: a raiz do projeto de
# cada máquina, ou um snapshot baseline e a raiz atual).
#
# Saída: tables/comparison.md (e no stdout) com:
#   - verificação de igualdade das métricas determinísticas (PASS/FAIL)
#   - speedup de tempo por experimento (B em relação a A)
# =============================================================================
import csv
import os
import sys

DET_COLS = ["disk_accesses", "avg_io_per_op", "height",
            "total_nodes", "free_nodes", "file_bytes"]
TIME_COLS = ["wall_ms", "cpu_user_ms", "cpu_sys_ms"]
KEY = ["label", "m", "N", "mode", "reuse", "phase"]
DATASETS = ["results_order_m_impact", "results_set_size_scaling",
            "results_node_reuse"]


def load(path):
    rows = {}
    if not os.path.exists(path):
        return rows
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            rows[tuple(r[k] for k in KEY)] = r
    return rows


def numeq(a, b, tol=1e-6):
    try:
        fa, fb = float(a), float(b)
    except (ValueError, TypeError):
        return a == b
    if fa == fb:
        return True
    denom = max(abs(fa), abs(fb), 1.0)
    return abs(fa - fb) / denom < tol


def main():
    if len(sys.argv) != 3:
        print("uso: python3 experiments/compare.py <dir_A> <dir_B>")
        sys.exit(2)
    dirA, dirB = sys.argv[1], sys.argv[2]
    nameA = os.path.basename(os.path.normpath(dirA)) or "A"
    nameB = os.path.basename(os.path.normpath(dirB)) or "B"

    out = [f"# Comparação de resultados: {nameA} (A) × {nameB} (B)\n",
           "Métricas estruturais devem ser **idênticas** (determinísticas); "
           "tempos variam com o hardware.\n"]
    total_mismatch = 0
    total_checked = 0
    speedups = []

    for ds in DATASETS:
        A = load(os.path.join(dirA, ds, "data.csv"))
        B = load(os.path.join(dirB, ds, "data.csv"))
        common = sorted(set(A) & set(B))
        only_a, only_b = set(A) - set(B), set(B) - set(A)
        out.append(f"\n## {ds}\n")
        out.append(f"- linhas em comum: {len(common)} | só em A: "
                   f"{len(only_a)} | só em B: {len(only_b)}\n")

        mism = []
        for k in common:
            for c in DET_COLS:
                if A[k].get(c, "") == "" and B[k].get(c, "") == "":
                    continue
                total_checked += 1
                if not numeq(A[k].get(c), B[k].get(c)):
                    total_mismatch += 1
                    mism.append((k, c, A[k].get(c), B[k].get(c)))
            # speedup de tempo (wall) — só onde A tem tempo > 0
            try:
                wa, wb = float(A[k]["wall_ms"]), float(B[k]["wall_ms"])
                if wa > 0:
                    speedups.append((ds, k, wa, wb, wb / wa))
            except (ValueError, KeyError):
                pass

        if mism:
            out.append("\n**DIVERGÊNCIAS nas métricas determinísticas "
                       "(deveriam ser iguais!):**\n\n")
            out.append("| chave (label,m,N,mode,reuse,phase) | métrica | A | B |\n")
            out.append("| --- | --- | --- | --- |\n")
            for k, c, va, vb in mism[:40]:
                out.append(f"| {','.join(k)} | {c} | {va} | {vb} |\n")
            if len(mism) > 40:
                out.append(f"| … +{len(mism)-40} divergências | | | |\n")
        else:
            out.append("\n✅ métricas determinísticas **idênticas** "
                       "em todas as linhas em comum.\n")

    # Resumo de tempo: speedup médio por dataset
    out.append("\n## Tempo (wall) — razão B/A (>1 = B mais lento)\n\n")
    out.append("| dataset | nº linhas | razão B/A mediana | mín | máx |\n")
    out.append("| --- | --- | --- | --- | --- |\n")
    by_ds = {}
    for ds, k, wa, wb, ratio in speedups:
        by_ds.setdefault(ds, []).append(ratio)
    for ds, ratios in by_ds.items():
        ratios.sort()
        med = ratios[len(ratios) // 2]
        out.append(f"| {ds} | {len(ratios)} | {med:.2f}× | "
                   f"{min(ratios):.2f}× | {max(ratios):.2f}× |\n")

    verdict = ("PASS — implementação portável e determinística"
               if total_mismatch == 0 else
               f"FAIL — {total_mismatch}/{total_checked} valores divergiram")
    out.append(f"\n## Veredito: **{verdict}**\n")
    out.append(f"\n(Comparados {total_checked} valores estruturais; "
               f"{total_mismatch} divergências.)\n")

    os.makedirs("tables", exist_ok=True)
    dst = os.path.join("tables", "comparison.md")
    with open(dst, "w") as f:
        f.write("".join(out))
    print("".join(out))
    print(f"\n>> salvo em {dst}")
    sys.exit(0 if total_mismatch == 0 else 1)


if __name__ == "__main__":
    main()
