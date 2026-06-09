#!/usr/bin/env python3
# =============================================================================
# build_presentation.py — Gera a apresentação final (PPTX + PDF) da Árvore B,
# seguindo o modelo de slides do enunciado (Slides 1–10) e populando com os
# RESULTADOS REAIS dos experimentos (pastas results_*/ e _baseline/).
#
# Funde a intenção do guia APRESENTACAO_PPT.md (btree-main) com os dados reais
# desta implementação. Saída em trabalho/apresentacao/.
#
# Requer (venv): python-pptx, reportlab, matplotlib.
# Uso: python3 experiments/build_presentation.py
# =============================================================================
import csv
import os
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "apresentacao")
ASSETS = os.path.join(OUT, "assets")
GRAPHS = os.path.join(ROOT, "results_graphs")
os.makedirs(ASSETS, exist_ok=True)

ACCENT = "#1d4ed8"   # azul
ACCENT2 = "#dc2626"  # vermelho
INK = "#111827"
MUTED = "#6b7280"


# --------------------------------------------------------------------------- #
# Carregamento de dados
# --------------------------------------------------------------------------- #
def load(path):
    rows = []
    if not os.path.exists(path):
        return rows
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            for k in ("m", "N", "disk_accesses", "height", "total_nodes",
                      "free_nodes", "file_bytes"):
                if r.get(k, "") != "":
                    r[k] = int(float(r[k]))
            for k in ("avg_io_per_op", "wall_ms", "cpu_user_ms",
                      "cpu_sys_ms", "io_wait_ms"):
                if r.get(k, "") != "":
                    r[k] = float(r[k])
            rows.append(r)
    return rows


ORDER = load(os.path.join(ROOT, "results_order_m_impact", "data.csv"))
SCALE = load(os.path.join(ROOT, "results_set_size_scaling", "data.csv"))
REUSE = load(os.path.join(ROOT, "results_node_reuse", "data.csv"))
ORDER_LAP = load(os.path.join(ROOT, "_baseline", "results_order_m_impact", "data.csv"))


def by_phase(rows, phase, mode=None):
    out = []
    for r in rows:
        if r["phase"] != phase:
            continue
        if mode and r["mode"] != mode:
            continue
        out.append(r)
    return out


# --------------------------------------------------------------------------- #
# Gráficos (matplotlib -> PNG)
# --------------------------------------------------------------------------- #
def chart_io_vs_m():
    fig, ax = plt.subplots(figsize=(7.2, 4.2), dpi=150)
    for mode, color, lbl in (("rand", ACCENT, "aleatório"),
                             ("seq", ACCENT2, "sequencial")):
        pts = sorted((r["m"], r["avg_io_per_op"])
                     for r in by_phase(ORDER, "search", mode))
        if pts:
            xs, ys = zip(*pts)
            ax.plot(xs, ys, "o-", color=color, label=lbl, linewidth=2)
    ax.set_xscale("log")
    ax.set_xlabel("ordem m (escala log)")
    ax.set_ylabel("I/O médio por busca")
    ax.set_title("Acessos a disco por busca vs ordem m  (N = 100.000)")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    p = os.path.join(ASSETS, "chart_io_vs_m.png")
    fig.savefig(p); plt.close(fig)
    return p


def chart_height_vs_m():
    fig, ax = plt.subplots(figsize=(7.2, 4.2), dpi=150)
    for mode, color, lbl in (("rand", ACCENT, "aleatório"),
                             ("seq", ACCENT2, "sequencial")):
        pts = sorted((r["m"], r["height"])
                     for r in by_phase(ORDER, "insert", mode))
        if pts:
            xs, ys = zip(*pts)
            ax.plot(xs, ys, "s-", color=color, label=lbl, linewidth=2)
    ax.set_xscale("log")
    ax.set_xlabel("ordem m (escala log)")
    ax.set_ylabel("altura da árvore (níveis)")
    ax.set_title("Altura da árvore vs ordem m  (N = 100.000)")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    p = os.path.join(ASSETS, "chart_height_vs_m.png")
    fig.savefig(p); plt.close(fig)
    return p


def chart_io_vs_N():
    fig, ax = plt.subplots(figsize=(7.2, 4.2), dpi=150)
    colors = {3: ACCENT, 100: "#059669", 1000: ACCENT2}
    for m in (3, 100, 1000):
        pts = sorted((r["N"], r["avg_io_per_op"]) for r in by_phase(SCALE, "search", "rand")
                     if r["m"] == m)
        if pts:
            xs, ys = zip(*pts)
            ax.plot(xs, ys, "o-", color=colors[m], label=f"m = {m}", linewidth=2)
    ax.set_xscale("log")
    ax.set_xlabel("N — nº de chaves (escala log)")
    ax.set_ylabel("I/O médio por busca")
    ax.set_title("Escalabilidade: I/O por busca vs N  (aleatório)")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    p = os.path.join(ASSETS, "chart_io_vs_N.png")
    fig.savefig(p); plt.close(fig)
    return p


def chart_reuse():
    ref = [r for r in REUSE if r["phase"] == "churn_refill"]
    by = defaultdict(dict)
    for r in ref:
        by[(r["m"], r["N"])][r["reuse"]] = r
    keys = sorted(by)
    labels = [f"m={m}\nN={N:,}" for (m, N) in keys]
    off = [by[k].get("0", {}).get("file_bytes", 0) / 1024 for k in keys]
    on = [by[k].get("1", {}).get("file_bytes", 0) / 1024 for k in keys]
    import numpy as np
    x = np.arange(len(keys)); w = 0.38
    fig, ax = plt.subplots(figsize=(7.2, 4.2), dpi=150)
    ax.bar(x - w/2, off, w, label="sem reaproveitamento", color=ACCENT2)
    ax.bar(x + w/2, on, w, label="com reaproveitamento", color=ACCENT)
    ax.set_xticks(x); ax.set_xticklabels(labels, fontsize=8)
    ax.set_ylabel("tamanho do arquivo (KB)")
    ax.set_title("Ocupação do arquivo após churn: com vs sem free list")
    ax.grid(True, axis="y", alpha=0.3)
    ax.legend()
    fig.tight_layout()
    p = os.path.join(ASSETS, "chart_reuse.png")
    fig.savefig(p); plt.close(fig)
    return p


def chart_machines():
    # tempo total (ins+busca+del) por m, notebook vs titan, N=100k rand
    def total_by_m(rows):
        tot = defaultdict(float)
        for r in rows:
            if r["mode"] == "rand" and r["phase"] in ("insert", "search", "delete"):
                tot[r["m"]] += r["wall_ms"]
        return tot
    lap = total_by_m(ORDER_LAP)
    tit = total_by_m(ORDER)
    ms = sorted(set(lap) & set(tit))
    if not ms:
        return None
    import numpy as np
    x = np.arange(len(ms)); w = 0.38
    fig, ax = plt.subplots(figsize=(7.2, 4.2), dpi=150)
    ax.bar(x - w/2, [lap[m]/1000 for m in ms], w, label="Notebook", color=ACCENT)
    ax.bar(x + w/2, [tit[m]/1000 for m in ms], w, label="Titan (USP)", color="#7c3aed")
    ax.set_xticks(x); ax.set_xticklabels([str(m) for m in ms], fontsize=8)
    ax.set_xlabel("ordem m")
    ax.set_ylabel("tempo total ins+busca+rem (s)")
    ax.set_title("Tempo por máquina (N=100k, aleatório) — métricas de disco idênticas")
    ax.grid(True, axis="y", alpha=0.3)
    ax.legend()
    fig.tight_layout()
    p = os.path.join(ASSETS, "chart_machines.png")
    fig.savefig(p); plt.close(fig)
    return p


print("gerando gráficos...")
C_IO_M = chart_io_vs_m()
C_H_M = chart_height_vs_m()
C_IO_N = chart_io_vs_N()
C_REUSE = chart_reuse()
C_MACH = chart_machines()


# --------------------------------------------------------------------------- #
# Tabelas dinâmicas (a partir dos dados reais)
# --------------------------------------------------------------------------- #
def order_table():
    by_m = defaultdict(dict)
    for r in ORDER:
        if r["mode"] == "rand":
            by_m[r["m"]][r["phase"]] = r
    headers = ["m", "altura", "busca I/O/op", "inser. I/O/op", "rem. I/O/op", "arquivo (KB)"]
    rows = []
    for m in sorted(by_m):
        d = by_m[m]
        rows.append([str(m), str(d["insert"]["height"]),
                     f'{d["search"]["avg_io_per_op"]:.2f}',
                     f'{d["insert"]["avg_io_per_op"]:.2f}',
                     f'{d["delete"]["avg_io_per_op"]:.2f}',
                     f'{d["insert"]["file_bytes"]/1024:,.0f}'])
    return headers, rows


def reuse_table():
    ref = [r for r in REUSE if r["phase"] == "churn_refill"]
    by = defaultdict(dict)
    for r in ref:
        by[(r["m"], r["N"])][r["reuse"]] = r
    headers = ["m", "N", "sem reuso (KB)", "com reuso (KB)", "economia"]
    rows = []
    for (m, N) in sorted(by):
        on = by[(m, N)].get("1"); off = by[(m, N)].get("0")
        if on and off and off["file_bytes"]:
            save = (1 - on["file_bytes"]/off["file_bytes"]) * 100
            rows.append([str(m), f"{N:,}",
                         f'{off["file_bytes"]/1024:,.0f}',
                         f'{on["file_bytes"]/1024:,.0f}', f"{save:.0f}%"])
    return headers, rows


ORDER_HEADERS, ORDER_ROWS = order_table()
REUSE_HEADERS, REUSE_ROWS = reuse_table()


# --------------------------------------------------------------------------- #
# Modelo de slides (compartilhado por PPTX e PDF)
# --------------------------------------------------------------------------- #
def G(name):
    return os.path.join(GRAPHS, name)

SLIDES = [
    {   # 1
        "title": "Árvore B em Memória Secundária",
        "subtitle": "Implementação de uma classe Árvore B de ordem m operando estritamente em disco",
        "cover": True,
        "bullets": [
            "Algoritmos e Estruturas de Dados — AED-PG-2026 (5955001-3)",
            "Prof. Dr. José Augusto Baranauskas — DCM / USP",
            "Linguagem: C++17  ·  1º Semestre/2026",
            "Aluno: Cássio Takarada   (demais integrantes: ____________)",
        ],
    },
    {   # 2
        "title": "2. Decisões de implementação",
        "bullets": [
            "Ordem m é constante simbólica de compilação (M): estrutura totalmente parametrizada.",
            "Raiz da árvore: armazenada no header (registro 0) do arquivo — campo root.",
            "Um nó por I/O: readNode/writeNode de um registro de PAGE_SIZE; a árvore NUNCA é carregada inteira em memória.",
            "Registro do nó: n  ·  A[0..m-1] (ponteiros RRN)  ·  K[0..m-1] (chaves).",
            "Reaproveitamento de nós: free list encadeada no próprio arquivo (free_head no header; nó livre marca n=-1 e guarda o próximo em K[1]).",
            "allocNode reutiliza a free list antes de estender o arquivo.",
        ],
        "images": [(G("m3_final.png"), "Árvore B (m=3) — 40 chaves; cada nó traz seu RRN em disco")],
    },
    {   # 3
        "title": "3. Métodos implementados",
        "bullets": [
            "mSearch / mSearchPath - busca m-way da raiz à folha; retorna (RRN, índice, achou).",
            "insertB - inserção bottom-up com propagação de split (Equação 1).",
            "deleteB - remoção com sucessor in-order, redistribuição (empréstimo) e fusão (merge).",
            "Auxiliares: splitNode, insertInNode, removeFromNode, findSuccessorLeaf.",
            "Monitoramento: printTree (hierárquico), height, exportDot (Graphviz).",
            "DiskManager: readNode, writeNode, allocNode, freeNode, contador de acessos, setReuse.",
            "bench - driver experimental não interativo (CSV de métricas).",
        ],
    },
    {   # 4
        "title": "4. Análises efetuadas (experimentos)",
        "bullets": [
            "Exp. 1 - Impacto da ordem m: m em {3,4,5,8,16,32,64,100,128,256,512,1000}, N=10^5.",
            "Exp. 2 - Escala do conjunto: N em {10^3,10^4,10^5,10^6}, m em {3,100,1000}.",
            "Exp. 3 - Ocupação do arquivo: workload de churn COM e SEM reaproveitamento.",
            "Exp. 4 - Tempo de execução: CPU (user+sys) vs espera de I/O (getrusage).",
            "Modos aleatório e sequencial em todos os experimentos.",
            "Novidade: validação cruzada em 2 máquinas (Notebook x Titan/USP) - métricas de disco idênticas.",
        ],
        "images": [(C_IO_N, None)],
    },
    {   # 5
        "title": "5. Métricas utilizadas",
        "bullets": [
            "Acessos ao disco - contador de readNode+writeNode; total e média por operação (a métrica honesta de I/O).",
            "Altura da árvore - nº de níveis (~ log_m N).",
            "Ocupação do arquivo - nós físicos (total_nodes), nós livres (free_nodes) e tamanho em bytes.",
            "Tempo de parede (wall) - relógio de alta resolução.",
            "Tempo de CPU - usuário (lógica) e sistema (syscalls de I/O), via getrusage.",
            "Espera de I/O - wall menos CPU.",
        ],
        "images": [(C_H_M, None)],
    },
    {   # 6
        "title": "6. Tabela de Resultados - impacto de m (N=10^5, aleatório)",
        "table": (ORDER_HEADERS, ORDER_ROWS),
        "images": [(C_IO_M, None)],
        "footnote": "I/O por busca cai de 13,25 (m=3, altura 14) para 2,00 (m>=512, altura 2). Ganho satura por volta de m~64-128.",
    },
    {   # 7
        "title": "6b. Resultados - reaproveitamento de nós (churn)",
        "table": (REUSE_HEADERS, REUSE_ROWS),
        "images": [(C_REUSE, None)],
        "footnote": "O reaproveitamento de nós economiza ~27-30% do tamanho do arquivo neste workload.",
    },
    {   # 8
        "title": "7. Utilização de LLM",
        "bullets": [
            "Ferramenta: Claude (Anthropic) via Claude Code CLI - auxílio ao desenvolvimento.",
            "Síntese e discussão dos pseudocódigos dos slides do professor.",
            "Identificação e correção de 2 bugs: (1) stale-header em insertB; (2) eofbit do fstream em escritas além do fim.",
            "Geração do harness experimental (bench), tabelas, gráficos e desta apresentação.",
            "Todo o código foi revisado e validado manualmente pelo autor.",
        ],
        "images": [(C_MACH, None)] if C_MACH else [],
    },
    {   # 9
        "title": "8. Dificuldades · Vantagens x Desvantagens",
        "bullets": [
            "Dificuldades: indexação 1-based dos nós; ordem do path no reparo de underflow; eofbit do fstream; garantir 1 nó por I/O (nada em RAM).",
            "Vantagens da Árvore B: altura baixa => pouquíssimos acessos a disco; sempre balanceada; ideal para memória secundária / SGBDs.",
            "Desvantagens: nós podem ficar subocupados (~50% no caso sequencial); remoção é complexa; para m muito grande, a busca dentro do nó passa a custar CPU.",
            "Trade-off central: m maior => menos I/O, mais CPU por nó.",
        ],
    },
    {   # 10
        "title": "9-10. Aplicações práticas · Conclusão · Referências",
        "bullets": [
            "Aplicações: índices de SGBDs (InnoDB/MySQL, PostgreSQL), sistemas de arquivos (NTFS, HFS+, ext4 HTree), key-value stores.",
            "Conclusão: o I/O por operação cai com m até saturar (m~64-128); o reaproveitamento economiza ~27-30% de espaço; resultados determinísticos validados em 2 máquinas.",
            "Referências:",
            "   - Comer, D. (1979). The Ubiquitous B-Tree. ACM Computing Surveys.",
            "   - Knuth, D. The Art of Computer Programming, Vol. 3 - Sorting and Searching.",
            "   - Folk, M. & Zoellick, B. File Structures.",
            "   - Baranauskas, J. A. Slides AED-PG-2026 (Árvores B).",
        ],
    },
]


# --------------------------------------------------------------------------- #
# Renderizador PPTX
# --------------------------------------------------------------------------- #
def build_pptx(path):
    from pptx import Presentation
    from pptx.util import Inches, Pt, Emu
    from pptx.dml.color import RGBColor
    from pptx.enum.text import PP_ALIGN, MSO_ANCHOR

    def rgb(h):
        return RGBColor.from_string(h.lstrip("#"))

    prs = Presentation()
    prs.slide_width = Inches(13.333)
    prs.slide_height = Inches(7.5)
    SW, SH = prs.slide_width, prs.slide_height
    blank = prs.slide_layouts[6]

    def add_text(slide, left, top, width, height, runs, anchor=MSO_ANCHOR.TOP):
        tb = slide.shapes.add_textbox(left, top, width, height)
        tf = tb.text_frame; tf.word_wrap = True; tf.vertical_anchor = anchor
        first = True
        for (text, size, color, bold, bullet) in runs:
            p = tf.paragraphs[0] if first else tf.add_paragraph()
            first = False
            p.space_after = Pt(6)
            r = p.add_run(); r.text = ("•  " + text) if bullet else text
            r.font.size = Pt(size); r.font.bold = bold; r.font.color.rgb = rgb(color)
            r.font.name = "Calibri"
        return tb

    def add_table(slide, headers, rows, left, top, width, height):
        nr, nc = len(rows) + 1, len(headers)
        gtbl = slide.shapes.add_table(nr, nc, left, top, width, height).table
        for j, h in enumerate(headers):
            c = gtbl.cell(0, j); c.text = h
            pr = c.text_frame.paragraphs[0]; pr.alignment = PP_ALIGN.CENTER
            run = pr.runs[0]; run.font.size = Pt(11); run.font.bold = True
            run.font.color.rgb = rgb("#ffffff")
            c.fill.solid(); c.fill.fore_color.rgb = rgb(ACCENT)
        for i, row in enumerate(rows, start=1):
            for j, val in enumerate(row):
                c = gtbl.cell(i, j); c.text = str(val)
                pr = c.text_frame.paragraphs[0]
                pr.alignment = PP_ALIGN.CENTER if j > 0 else PP_ALIGN.LEFT
                run = pr.runs[0]; run.font.size = Pt(10); run.font.color.rgb = rgb(INK)
                c.fill.solid()
                c.fill.fore_color.rgb = rgb("#eef2ff" if i % 2 == 0 else "#ffffff")
        return gtbl

    def add_image_fit(slide, img, left, top, max_w, max_h, caption=None):
        from PIL import Image
        try:
            iw, ih = Image.open(img).size
        except Exception:
            iw, ih = (1280, 720)
        scale = min(max_w / iw, max_h / ih)
        w, h = int(iw * scale), int(ih * scale)
        l = left + (max_w - w) // 2
        slide.shapes.add_picture(img, l, top, width=w, height=h)
        if caption:
            add_text(slide, left, top + h + Emu(20000), max_w, Inches(0.4),
                     [(caption, 9, MUTED, False, False)])

    for idx, s in enumerate(SLIDES):
        slide = prs.slides.add_slide(blank)
        if s.get("cover"):
            band = slide.shapes.add_shape(1, 0, Inches(2.2), SW, Inches(1.7))
            band.fill.solid(); band.fill.fore_color.rgb = rgb(ACCENT); band.line.fill.background()
            add_text(slide, Inches(0.8), Inches(2.35), Inches(11.7), Inches(1.4),
                     [(s["title"], 40, "#ffffff", True, False)], anchor=MSO_ANCHOR.MIDDLE)
            add_text(slide, Inches(0.8), Inches(4.1), Inches(11.7), Inches(0.9),
                     [(s["subtitle"], 18, INK, False, False)])
            runs = [(b, 15, MUTED, False, False) for b in s["bullets"]]
            add_text(slide, Inches(0.8), Inches(5.0), Inches(11.7), Inches(2.0), runs)
            continue

        band = slide.shapes.add_shape(1, 0, 0, SW, Inches(0.95))
        band.fill.solid(); band.fill.fore_color.rgb = rgb(ACCENT); band.line.fill.background()
        add_text(slide, Inches(0.5), Inches(0.12), Inches(12.3), Inches(0.72),
                 [(s["title"], 24, "#ffffff", True, False)], anchor=MSO_ANCHOR.MIDDLE)

        body_top = Inches(1.2)
        if s.get("images") and s.get("bullets"):
            runs = [(b, 14, INK, False, True) for b in s["bullets"]]
            add_text(slide, Inches(0.5), body_top, Inches(7.0), Inches(5.8), runs)
            img, cap = s["images"][0]
            add_image_fit(slide, img, Inches(7.7), Inches(1.3), Inches(5.3), Inches(5.4), cap)
        elif s.get("table"):
            headers, rows = s["table"]
            add_table(slide, headers, rows, Inches(0.4), body_top, Inches(6.3), Inches(5.2))
            if s.get("images"):
                img, cap = s["images"][0]
                add_image_fit(slide, img, Inches(7.0), Inches(1.3), Inches(6.0), Inches(4.9), cap)
            if s.get("footnote"):
                add_text(slide, Inches(0.4), Inches(6.7), Inches(12.5), Inches(0.6),
                         [(s["footnote"], 11, MUTED, False, False)])
        else:
            runs = [(b, 16, INK, False, True) for b in s["bullets"]]
            add_text(slide, Inches(0.7), body_top, Inches(12.0), Inches(5.8), runs)

        add_text(slide, Inches(0.5), Inches(7.05), Inches(12.3), Inches(0.35),
                 [(f"Árvore B em Memória Secundária — AED-PG-2026          {idx+1}/{len(SLIDES)}",
                   9, MUTED, False, False)])

    prs.save(path)
    print("PPTX:", path)


# --------------------------------------------------------------------------- #
# Renderizador PDF (reportlab)
# --------------------------------------------------------------------------- #
def build_pdf(path):
    from reportlab.pdfgen import canvas
    from reportlab.lib.utils import ImageReader
    from reportlab.lib.colors import HexColor, white

    W, H = 960, 540  # 16:9 em pontos
    c = canvas.Canvas(path, pagesize=(W, H))

    def wrap(text, font, size, max_w):
        words = text.split(" ")
        lines, cur = [], ""
        for w in words:
            trial = (cur + " " + w).strip()
            if c.stringWidth(trial, font, size) <= max_w:
                cur = trial
            else:
                if cur:
                    lines.append(cur)
                cur = w
        if cur:
            lines.append(cur)
        return lines

    def draw_image_fit(img, x, y_top, max_w, max_h, caption=None):
        try:
            ir = ImageReader(img); iw, ih = ir.getSize()
        except Exception:
            return
        scale = min(max_w / iw, max_h / ih)
        w, h = iw * scale, ih * scale
        x2 = x + (max_w - w) / 2
        c.drawImage(ir, x2, y_top - h, width=w, height=h,
                    preserveAspectRatio=True, mask="auto")
        if caption:
            c.setFont("Helvetica", 8); c.setFillColor(HexColor(MUTED))
            c.drawCentredString(x + max_w / 2, y_top - h - 12, caption)

    def draw_table(headers, rows, x, y_top, total_w):
        nc = len(headers); cw = total_w / nc; rh = 22
        c.setFillColor(HexColor(ACCENT))
        c.rect(x, y_top - rh, total_w, rh, fill=1, stroke=0)
        c.setFillColor(white); c.setFont("Helvetica-Bold", 9.5)
        for j, hd in enumerate(headers):
            c.drawCentredString(x + cw * j + cw / 2, y_top - rh + 7, str(hd))
        c.setFont("Helvetica", 9)
        yy = y_top - rh
        for i, row in enumerate(rows):
            if i % 2 == 0:
                c.setFillColor(HexColor("#eef2ff"))
                c.rect(x, yy - rh, total_w, rh, fill=1, stroke=0)
            c.setFillColor(HexColor(INK))
            for j, val in enumerate(row):
                if j == 0:
                    c.drawString(x + 6, yy - rh + 7, str(val))
                else:
                    c.drawCentredString(x + cw * j + cw / 2, yy - rh + 7, str(val))
            yy -= rh
        return yy

    for idx, s in enumerate(SLIDES):
        if s.get("cover"):
            c.setFillColor(HexColor("#ffffff")); c.rect(0, 0, W, H, fill=1, stroke=0)
            c.setFillColor(HexColor(ACCENT)); c.rect(0, H - 250, W, 130, fill=1, stroke=0)
            c.setFillColor(white); c.setFont("Helvetica-Bold", 34)
            for k, ln in enumerate(wrap(s["title"], "Helvetica-Bold", 34, W - 120)):
                c.drawString(60, H - 175 - k * 38, ln)
            c.setFillColor(HexColor(INK)); c.setFont("Helvetica", 15)
            for k, ln in enumerate(wrap(s["subtitle"], "Helvetica", 15, W - 120)):
                c.drawString(60, H - 285 - k * 20, ln)
            c.setFillColor(HexColor(MUTED)); c.setFont("Helvetica", 13)
            for k, b in enumerate(s["bullets"]):
                c.drawString(60, H - 360 - k * 24, b)
            c.showPage()
            continue

        c.setFillColor(HexColor("#ffffff")); c.rect(0, 0, W, H, fill=1, stroke=0)
        c.setFillColor(HexColor(ACCENT)); c.rect(0, H - 64, W, 64, fill=1, stroke=0)
        c.setFillColor(white); c.setFont("Helvetica-Bold", 19)
        c.drawString(36, H - 42, s["title"])

        top = H - 92
        if s.get("images") and s.get("bullets"):
            yy = top
            for b in s["bullets"]:
                lines = wrap(b, "Helvetica", 12.5, 470)
                c.setFillColor(HexColor(ACCENT)); c.setFont("Helvetica", 12.5)
                c.drawString(36, yy, "•")
                c.setFillColor(HexColor(INK))
                for ln in lines:
                    c.drawString(52, yy, ln); yy -= 17
                yy -= 4
            img, cap = s["images"][0]
            draw_image_fit(img, 540, top + 6, 390, 400, cap)
        elif s.get("table"):
            headers, rows = s["table"]
            draw_table(headers, rows, 28, top, 430)
            if s.get("images"):
                img, cap = s["images"][0]
                draw_image_fit(img, 480, top, 450, 360, cap)
            if s.get("footnote"):
                c.setFillColor(HexColor(MUTED)); c.setFont("Helvetica-Oblique", 10)
                for k, ln in enumerate(wrap(s["footnote"], "Helvetica-Oblique", 10, W - 70)):
                    c.drawString(36, 60 - k * 13, ln)
        else:
            c.setFont("Helvetica", 14); yy = top
            for b in s["bullets"]:
                lines = wrap(b, "Helvetica", 14, W - 110)
                c.setFillColor(HexColor(ACCENT)); c.drawString(40, yy, "•")
                c.setFillColor(HexColor(INK))
                for ln in lines:
                    c.drawString(58, yy, ln); yy -= 20
                yy -= 6

        c.setFillColor(HexColor(MUTED)); c.setFont("Helvetica", 8)
        c.drawString(36, 16, "Árvore B em Memória Secundária — AED-PG-2026")
        c.drawRightString(W - 36, 16, f"{idx+1}/{len(SLIDES)}")
        c.showPage()

    c.save()
    print("PDF:", path)


if __name__ == "__main__":
    build_pptx(os.path.join(OUT, "APRESENTACAO.pptx"))
    build_pdf(os.path.join(OUT, "APRESENTACAO.pdf"))
    print("\nApresentação gerada em:", OUT)
