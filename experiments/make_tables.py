#!/usr/bin/env python3
# =============================================================================
# make_tables.py — Agrega os CSVs dos experimentos em tabelas comparativas
# (Markdown + CSV) na pasta tables/ e gera gráficos SVG sem dependências
# externas (apenas stdlib). Também monta a pasta results_cpu_vs_io/ como uma
# visão do tempo de CPU vs espera de I/O extraída do experimento de escala.
#
# Uso: python3 experiments/make_tables.py
# =============================================================================
import csv
import os
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TABLES = os.path.join(ROOT, "tables")
os.makedirs(TABLES, exist_ok=True)


def load(path):
    if not os.path.exists(path):
        return []
    with open(path, newline="") as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        for k in ("m", "N", "disk_accesses", "height", "total_nodes",
                  "free_nodes", "file_bytes"):
            if r.get(k, "") != "":
                r[k] = int(float(r[k]))
        for k in ("avg_io_per_op", "wall_ms", "cpu_user_ms",
                  "cpu_sys_ms", "io_wait_ms"):
            if r.get(k, "") != "":
                r[k] = float(r[k])
    return rows


def fmt(v):
    if isinstance(v, float):
        if v == 0:
            return "0"
        if abs(v) >= 100:
            return f"{v:,.0f}"
        if abs(v) >= 1:
            return f"{v:.2f}"
        return f"{v:.3f}"
    if isinstance(v, int):
        return f"{v:,}"
    return str(v)


def md_table(headers, rows):
    out = ["| " + " | ".join(headers) + " |",
           "| " + " | ".join("---" for _ in headers) + " |"]
    for r in rows:
        out.append("| " + " | ".join(fmt(c) for c in r) + " |")
    return "\n".join(out) + "\n"


def write_csv(path, headers, rows):
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(headers)
        for r in rows:
            w.writerow(r)


# ---------------------------------------------------------------------------
# Gráfico SVG mínimo (linha) — sem matplotlib. Eixo X categórico/logável.
# ---------------------------------------------------------------------------
PALETTE = ["#2563eb", "#dc2626", "#059669", "#d97706", "#7c3aed",
           "#0891b2", "#db2777", "#65a30d"]


def svg_line(path, series, xlabel, ylabel, title, xlog=False, ylog=False):
    # series: list of (name, [(x, y), ...])
    W, H = 760, 460
    ml, mr, mt, mb = 78, 150, 56, 64
    pw, ph = W - ml - mr, H - mt - mb
    import math

    xs = [x for _, pts in series for x, _ in pts]
    ys = [y for _, pts in series for _, y in pts if y is not None]
    if not xs or not ys:
        return
    xs_t = [math.log10(x) if xlog else x for x in xs]
    ys_t = [math.log10(y) if (ylog and y > 0) else y for y in ys]
    xmin, xmax = min(xs_t), max(xs_t)
    ymin, ymax = min(ys_t + [0]) if not ylog else min(ys_t), max(ys_t)
    if xmax == xmin:
        xmax += 1
    if ymax == ymin:
        ymax += 1
    pad = (ymax - ymin) * 0.08
    ymin -= pad
    ymax += pad

    def px(x):
        xt = math.log10(x) if xlog else x
        return ml + (xt - xmin) / (xmax - xmin) * pw

    def py(y):
        yt = math.log10(y) if (ylog and y > 0) else y
        return mt + ph - (yt - ymin) / (ymax - ymin) * ph

    s = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
         f'font-family="Helvetica,Arial,sans-serif" font-size="12">']
    s.append(f'<rect width="{W}" height="{H}" fill="white"/>')
    s.append(f'<text x="{W/2}" y="26" text-anchor="middle" '
             f'font-size="16" font-weight="bold">{title}</text>')
    # grid + y ticks
    for i in range(6):
        yv = ymin + (ymax - ymin) * i / 5
        yy = mt + ph - (yv - ymin) / (ymax - ymin) * ph
        s.append(f'<line x1="{ml}" y1="{yy:.1f}" x2="{ml+pw}" y2="{yy:.1f}" '
                 f'stroke="#e5e7eb"/>')
        lbl = 10 ** yv if ylog else yv
        s.append(f'<text x="{ml-8}" y="{yy+4:.1f}" text-anchor="end" '
                 f'fill="#374151">{lbl:,.1f}</text>')
    # x ticks
    xticks = sorted(set(xs))
    for xv in xticks:
        xx = px(xv)
        s.append(f'<line x1="{xx:.1f}" y1="{mt+ph}" x2="{xx:.1f}" '
                 f'y2="{mt+ph+5}" stroke="#374151"/>')
        s.append(f'<text x="{xx:.1f}" y="{mt+ph+20}" text-anchor="middle" '
                 f'fill="#374151">{xv:g}</text>')
    # axes
    s.append(f'<line x1="{ml}" y1="{mt}" x2="{ml}" y2="{mt+ph}" stroke="#111"/>')
    s.append(f'<line x1="{ml}" y1="{mt+ph}" x2="{ml+pw}" y2="{mt+ph}" stroke="#111"/>')
    s.append(f'<text x="{ml+pw/2}" y="{H-16}" text-anchor="middle" '
             f'fill="#111">{xlabel}</text>')
    s.append(f'<text x="18" y="{mt+ph/2}" text-anchor="middle" fill="#111" '
             f'transform="rotate(-90 18 {mt+ph/2})">{ylabel}</text>')
    # series
    for idx, (name, pts) in enumerate(series):
        color = PALETTE[idx % len(PALETTE)]
        pts = [(x, y) for x, y in pts if y is not None]
        if not pts:
            continue
        d = " ".join(f"{px(x):.1f},{py(y):.1f}" for x, y in pts)
        s.append(f'<polyline points="{d}" fill="none" stroke="{color}" '
                 f'stroke-width="2.2"/>')
        for x, y in pts:
            s.append(f'<circle cx="{px(x):.1f}" cy="{py(y):.1f}" r="3" '
                     f'fill="{color}"/>')
        ly = mt + 10 + idx * 20
        s.append(f'<line x1="{ml+pw+14}" y1="{ly}" x2="{ml+pw+34}" y2="{ly}" '
                 f'stroke="{color}" stroke-width="2.6"/>')
        s.append(f'<text x="{ml+pw+38}" y="{ly+4}" fill="#111">{name}</text>')
    s.append("</svg>")
    with open(path, "w") as f:
        f.write("\n".join(s))


def svg_bars(path, groups, series_names, title, ylabel):
    # groups: list of (label, [v_series0, v_series1, ...])
    W, H = 760, 460
    ml, mr, mt, mb = 78, 150, 56, 70
    pw, ph = W - ml - mr, H - mt - mb
    vals = [v for _, vs in groups for v in vs if v is not None]
    if not vals:
        return
    ymax = max(vals) * 1.12
    ng = len(groups)
    ns = len(series_names)
    gw = pw / max(ng, 1)
    bw = gw * 0.8 / max(ns, 1)

    def py(y):
        return mt + ph - (y / ymax) * ph

    s = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
         f'font-family="Helvetica,Arial,sans-serif" font-size="12">']
    s.append(f'<rect width="{W}" height="{H}" fill="white"/>')
    s.append(f'<text x="{W/2}" y="26" text-anchor="middle" font-size="16" '
             f'font-weight="bold">{title}</text>')
    for i in range(6):
        yv = ymax * i / 5
        yy = py(yv)
        s.append(f'<line x1="{ml}" y1="{yy:.1f}" x2="{ml+pw}" y2="{yy:.1f}" '
                 f'stroke="#e5e7eb"/>')
        s.append(f'<text x="{ml-8}" y="{yy+4:.1f}" text-anchor="end" '
                 f'fill="#374151">{yv:,.0f}</text>')
    s.append(f'<line x1="{ml}" y1="{mt}" x2="{ml}" y2="{mt+ph}" stroke="#111"/>')
    s.append(f'<line x1="{ml}" y1="{mt+ph}" x2="{ml+pw}" y2="{mt+ph}" stroke="#111"/>')
    s.append(f'<text x="18" y="{mt+ph/2}" text-anchor="middle" fill="#111" '
             f'transform="rotate(-90 18 {mt+ph/2})">{ylabel}</text>')
    for gi, (label, vs) in enumerate(groups):
        gx = ml + gi * gw + gw * 0.1
        for si, v in enumerate(vs):
            if v is None:
                continue
            color = PALETTE[si % len(PALETTE)]
            x = gx + si * bw
            yy = py(v)
            s.append(f'<rect x="{x:.1f}" y="{yy:.1f}" width="{bw*0.92:.1f}" '
                     f'height="{mt+ph-yy:.1f}" fill="{color}"/>')
        s.append(f'<text x="{gx+ (ns*bw)/2:.1f}" y="{mt+ph+18}" '
                 f'text-anchor="middle" fill="#374151">{label}</text>')
    for si, nm in enumerate(series_names):
        color = PALETTE[si % len(PALETTE)]
        ly = mt + 10 + si * 20
        s.append(f'<rect x="{ml+pw+14}" y="{ly-9}" width="16" height="12" '
                 f'fill="{color}"/>')
        s.append(f'<text x="{ml+pw+36}" y="{ly+2}" fill="#111">{nm}</text>')
    s.append("</svg>")
    with open(path, "w") as f:
        f.write("\n".join(s))


# ===========================================================================
# EXPERIMENTO 1 — impacto da ordem m
# ===========================================================================
def exp_order():
    rows = load(os.path.join(ROOT, "results_order_m_impact", "data.csv"))
    if not rows:
        print("! sem dados de order_m_impact")
        return
    # tabela: para mode=rand, por m, mostra height + io/op de cada fase
    headers = ["m", "altura", "ins_io/op", "busca_io/op", "del_io/op",
               "ins_ms", "busca_ms", "del_ms", "nós", "arquivo(KB)"]
    for mode in ("rand", "seq"):
        by_m = defaultdict(dict)
        for r in rows:
            if r["mode"] != mode:
                continue
            by_m[r["m"]][r["phase"]] = r
        trows = []
        for m in sorted(by_m):
            d = by_m[m]
            ins = d.get("insert", {})
            srch = d.get("search", {})
            dele = d.get("delete", {})
            trows.append([m, ins.get("height", ""),
                          ins.get("avg_io_per_op", ""),
                          srch.get("avg_io_per_op", ""),
                          dele.get("avg_io_per_op", ""),
                          ins.get("wall_ms", ""), srch.get("wall_ms", ""),
                          dele.get("wall_ms", ""),
                          ins.get("total_nodes", ""),
                          round(ins.get("file_bytes", 0) / 1024, 1)])
        title = "Aleatório" if mode == "rand" else "Sequencial"
        with open(os.path.join(TABLES, f"exp1_order_impact_{mode}.md"), "w") as f:
            f.write(f"# Experimento 1 — Impacto da ordem m (inserção {title}, "
                    f"N=100.000)\n\n")
            f.write(md_table(headers, trows))
        write_csv(os.path.join(TABLES, f"exp1_order_impact_{mode}.csv"),
                  headers, trows)

    # gráfico: io/op de busca vs m (rand) — onde o I/O despenca
    for phase, fname in (("search", "exp1_io_vs_m_busca"),
                         ("insert", "exp1_io_vs_m_insercao")):
        series = []
        for mode in ("rand", "seq"):
            pts = sorted((r["m"], r["avg_io_per_op"]) for r in rows
                         if r["phase"] == phase and r["mode"] == mode)
            series.append((mode, pts))
        ph_lbl = "busca" if phase == "search" else "inserção"
        svg_line(os.path.join(TABLES, f"{fname}.svg"), series,
                 "ordem m (escala log)", f"I/O médio por {ph_lbl}",
                 f"I/O médio por {ph_lbl} vs ordem m (N=100k)", xlog=True)

    # gráfico: altura vs m
    series = []
    for mode in ("rand", "seq"):
        pts = sorted((r["m"], r["height"]) for r in rows
                     if r["phase"] == "insert" and r["mode"] == mode)
        series.append((mode, pts))
    svg_line(os.path.join(TABLES, "exp1_altura_vs_m.svg"), series,
             "ordem m (escala log)", "altura da árvore (níveis)",
             "Altura da árvore vs ordem m (N=100k)", xlog=True)
    print("OK exp1")


# ===========================================================================
# EXPERIMENTO 2 — escala do conjunto
# ===========================================================================
def exp_scale():
    rows = load(os.path.join(ROOT, "results_set_size_scaling", "data.csv"))
    if not rows:
        print("! sem dados de set_size_scaling")
        return
    headers = ["m", "N", "modo", "altura", "ins_io/op", "busca_io/op",
               "del_io/op", "ins_ms", "busca_ms", "del_ms", "arquivo(KB)"]
    by = defaultdict(dict)
    for r in rows:
        by[(r["m"], r["N"], r["mode"])][r["phase"]] = r
    trows = []
    for (m, N, mode) in sorted(by):
        d = by[(m, N, mode)]
        ins = d.get("insert", {}); srch = d.get("search", {}); dele = d.get("delete", {})
        trows.append([m, N, mode, ins.get("height", ""),
                      ins.get("avg_io_per_op", ""), srch.get("avg_io_per_op", ""),
                      dele.get("avg_io_per_op", ""),
                      ins.get("wall_ms", ""), srch.get("wall_ms", ""),
                      dele.get("wall_ms", ""),
                      round(ins.get("file_bytes", 0) / 1024, 1)])
    with open(os.path.join(TABLES, "exp2_set_size_scaling.md"), "w") as f:
        f.write("# Experimento 2 — Escala do conjunto (10^3 a 10^6 chaves)\n\n")
        f.write(md_table(headers, trows))
    write_csv(os.path.join(TABLES, "exp2_set_size_scaling.csv"), headers, trows)

    # gráfico: busca io/op vs N para cada (m, mode)
    series = []
    combos = sorted(set((r["m"], r["mode"]) for r in rows))
    for (m, mode) in combos:
        pts = sorted((r["N"], r["avg_io_per_op"]) for r in rows
                     if r["phase"] == "search" and r["m"] == m and r["mode"] == mode)
        series.append((f"m={m} {mode}", pts))
    svg_line(os.path.join(TABLES, "exp2_busca_io_vs_N.svg"), series,
             "N (escala log)", "I/O médio por busca",
             "I/O médio de busca vs tamanho do conjunto", xlog=True)

    # gráfico: tempo total (ins+busca+del) vs N
    series = []
    for (m, mode) in combos:
        pts = []
        for N in sorted(set(r["N"] for r in rows)):
            tot = sum(r["wall_ms"] for r in rows
                      if r["m"] == m and r["mode"] == mode and r["N"] == N
                      and r["phase"] in ("insert", "search", "delete"))
            if tot:
                pts.append((N, tot))
        series.append((f"m={m} {mode}", pts))
    svg_line(os.path.join(TABLES, "exp2_tempo_vs_N.svg"), series,
             "N (escala log)", "tempo total (ms)",
             "Tempo total (ins+busca+rem) vs N", xlog=True, ylog=True)
    print("OK exp2")


# ===========================================================================
# EXPERIMENTO 3 — ocupação do arquivo / reaproveitamento
# ===========================================================================
def exp_reuse():
    rows = load(os.path.join(ROOT, "results_node_reuse", "data.csv"))
    if not rows:
        print("! sem dados de node_reuse")
        return
    ref = [r for r in rows if r["phase"] == "churn_refill"]
    headers = ["m", "N", "reuse", "nós_físicos", "nós_livres",
               "arquivo(KB)", "refill_io/op"]
    trows = []
    for r in sorted(ref, key=lambda r: (r["m"], r["N"], -int(r["reuse"]))):
        trows.append([r["m"], r["N"], "sim" if r["reuse"] == "1" else "não",
                      r["total_nodes"], r["free_nodes"],
                      round(r["file_bytes"] / 1024, 1), r["avg_io_per_op"]])
    # linha de economia
    econ = []
    by = defaultdict(dict)
    for r in ref:
        by[(r["m"], r["N"])][r["reuse"]] = r
    for (m, N) in sorted(by):
        on = by[(m, N)].get("1"); off = by[(m, N)].get("0")
        if on and off and off["file_bytes"]:
            save = (1 - on["file_bytes"] / off["file_bytes"]) * 100
            econ.append([m, N, round(off["file_bytes"]/1024, 1),
                         round(on["file_bytes"]/1024, 1), round(save, 1)])
    with open(os.path.join(TABLES, "exp3_node_reuse.md"), "w") as f:
        f.write("# Experimento 3 — Ocupação do arquivo: com vs sem "
                "reaproveitamento de nós\n\n")
        f.write("Workload *churn*: insere N → remove metade → reinsere "
                "metade de chaves novas.\n\n")
        f.write(md_table(headers, trows))
        f.write("\n## Economia de espaço com reaproveitamento\n\n")
        f.write(md_table(["m", "N", "sem reuso(KB)", "com reuso(KB)",
                          "economia(%)"], econ))
    write_csv(os.path.join(TABLES, "exp3_node_reuse.csv"), headers, trows)

    # gráfico de barras: arquivo KB com vs sem reuso
    groups = []
    for (m, N) in sorted(by):
        on = by[(m, N)].get("1"); off = by[(m, N)].get("0")
        groups.append((f"m={m}\nN={N}",
                       [off["file_bytes"]/1024 if off else None,
                        on["file_bytes"]/1024 if on else None]))
    svg_bars(os.path.join(TABLES, "exp3_arquivo_reuso.svg"), groups,
             ["sem reuso", "com reuso"],
             "Tamanho do arquivo após churn (KB)", "tamanho (KB)")
    print("OK exp3")


# ===========================================================================
# VISÃO CPU vs I/O — extraída do experimento de escala
# ===========================================================================
def view_cpu_io():
    rows = load(os.path.join(ROOT, "results_set_size_scaling", "data.csv"))
    if not rows:
        return
    dst = os.path.join(ROOT, "results_cpu_vs_io")
    os.makedirs(dst, exist_ok=True)
    headers = ["m", "N", "modo", "fase", "wall_ms", "cpu_user_ms",
               "cpu_sys_ms", "io_wait_ms", "%cpu", "%io"]
    trows = []
    for r in sorted(rows, key=lambda r: (r["m"], r["N"], r["mode"], r["phase"])):
        if r["mode"] != "rand":
            continue
        wall = r["wall_ms"] or 1e-9
        cpu = r["cpu_user_ms"] + r["cpu_sys_ms"]
        # Em fases curtas (sub-100ms) a granularidade do getrusage faz cpu>wall;
        # limita a 100% para a leitura ficar coerente (cpu% + io% = 100).
        cpu_pct = min(cpu / wall * 100, 100.0)
        io_pct = 100.0 - cpu_pct
        trows.append([r["m"], r["N"], r["mode"], r["phase"],
                      r["wall_ms"], r["cpu_user_ms"], r["cpu_sys_ms"],
                      r["io_wait_ms"], round(cpu_pct, 1), round(io_pct, 1)])
    write_csv(os.path.join(dst, "data.csv"), headers, trows)
    with open(os.path.join(TABLES, "exp4_cpu_vs_io.md"), "w") as f:
        f.write("# Experimento 4 — Tempo de CPU vs espera de I/O "
                "(aleatório)\n\n")
        f.write("`cpu = user + sys` (sys captura as syscalls de leitura/escrita "
                "de nós); `io_wait = wall - cpu`.\n\n")
        f.write("> Em fases curtas (N pequeno) a granularidade do relógio de CPU "
                "(`getrusage`) é da ordem do próprio tempo medido, então `%cpu` é "
                "limitado a 100%. As linhas confiáveis para a razão CPU×I/O são as "
                "de **N = 10⁶**, onde fica claro que ~98% do tempo é CPU "
                "(predominância da CPU de sistema, pois o `flush` vai ao cache de "
                "páginas do SO, não ao disco físico).\n\n")
        f.write(md_table(headers, trows))
    write_csv(os.path.join(TABLES, "exp4_cpu_vs_io.csv"), headers, trows)
    print("OK exp4 (cpu vs io)")


if __name__ == "__main__":
    exp_order()
    exp_scale()
    exp_reuse()
    view_cpu_io()
    print(f"\nTabelas e gráficos em: {TABLES}")
