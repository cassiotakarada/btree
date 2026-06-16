#!/usr/bin/env python3
"""Gera GIFs animados demonstrando a Árvore B rodando, do `make M=<m>` até cada
item do menu, encerrando com o grafo gerado pelo próprio programa.

Saída: apresentacao/assets/demo_m3.gif, demo_m5.gif e demo_btree.gif (m3+m5).
As transcrições são REAIS — capturadas em experiments/gif_assets/ por sessões
roteirizadas do binário. Aqui apenas renderizamos.
"""
import os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GA = os.path.join(ROOT, "experiments", "gif_assets")
OUT = os.path.join(ROOT, "apresentacao", "assets")
os.makedirs(OUT, exist_ok=True)

# ---- terminal "look" -------------------------------------------------------
W, H = 1040, 660
PAD = 26
COLS_ROWS = 30            # linhas visíveis (terminal "rola")
BG = (13, 17, 23)         # fundo (GitHub dark)
BAR = (32, 38, 48)        # barra de título
FG = (201, 209, 217)      # texto padrão
GREEN = (63, 185, 80)     # prompt / sucesso
CYAN = (88, 166, 255)     # títulos de menu
YELLOW = (227, 179, 65)   # números/medidas
MUTED = (110, 118, 129)

FONT_DIR = "/usr/share/fonts/truetype/dejavu"
MONO = ImageFont.truetype(os.path.join(FONT_DIR, "DejaVuSansMono.ttf"), 18)
MONO_B = ImageFont.truetype(os.path.join(FONT_DIR, "DejaVuSansMono-Bold.ttf"), 18)
SANS_B = ImageFont.truetype(os.path.join(FONT_DIR, "DejaVuSans-Bold.ttf"), 22)
SANS = ImageFont.truetype(os.path.join(FONT_DIR, "DejaVuSans.ttf"), 17)
LINE_H = 20


def colorize(line):
    """Escolhe a cor de uma linha de transcrição para dar vida ao terminal."""
    s = line.lstrip()
    if line.startswith("$ ") or line.startswith("> ") or line.endswith("> "):
        return GREEN
    if s.startswith("===") or s.startswith("---"):
        return CYAN
    if s.startswith(("Inserido", "Encontrado", "Removido", "Gerado", "BUILD", "OK")):
        return GREEN
    if s.startswith(("Acessos", "Tempo", "[RRN")):
        return YELLOW
    return FG


def term_frame(title, lines):
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)
    # barra de título estilo macOS
    d.rectangle([0, 0, W, 44], fill=BAR)
    for i, c in enumerate([(255, 95, 86), (255, 189, 46), (39, 201, 63)]):
        d.ellipse([PAD + i * 26, 15, PAD + i * 26 + 14, 29], fill=c)
    d.text((W / 2, 22), title, font=MONO_B, fill=FG, anchor="mm")
    # corpo (mostra as últimas COLS_ROWS linhas — terminal rolando)
    visible = lines[-COLS_ROWS:]
    y = 58
    for ln in visible:
        d.text((PAD, y), ln, font=MONO, fill=colorize(ln))
        y += LINE_H
    return img


def graph_frame(title, subtitle, png_path):
    img = Image.new("RGB", (W, H), (255, 255, 255))
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, W, 70], fill=(29, 78, 216))
    d.text((PAD, 35), title, font=SANS_B, fill=(255, 255, 255), anchor="lm")
    g = Image.open(png_path).convert("RGB")
    maxw, maxh = W - 2 * PAD, H - 170
    scale = min(maxw / g.width, maxh / g.height)
    g = g.resize((int(g.width * scale), int(g.height * scale)))
    img.paste(g, ((W - g.width) // 2, 95))
    d.text((W / 2, H - 48), subtitle, font=SANS, fill=(75, 85, 99), anchor="mm")
    d.text((W / 2, H - 22),
           "exportado pelo proprio programa  ->  exportDot()  +  Graphviz (dot)",
           font=SANS, fill=MUTED, anchor="mm")
    return img


def read(name):
    with open(os.path.join(GA, name), encoding="utf-8", errors="replace") as f:
        return f.read().replace("\t", "    ").splitlines()


def build_gif_for(m, build_file, session_file, graph_png, out_name):
    title = f"Arvore B (ordem m={m})"
    build = read(build_file)
    sess = read(session_file)

    frames, durations = [], []

    def push(img, ms):
        frames.append(img.convert("P", palette=Image.ADAPTIVE, colors=128))
        durations.append(ms)

    # ---- frame 0: o make ----
    push(term_frame(f"{title} — compilando", [f"$ make M={m}"] + build[1:] +
                    ["", f"OK: binario ./btree gerado para ordem m={m}"]), 2200)

    # ---- frames: sessão acumulada; um snapshot a cada AÇÃO concluída ----
    # (cada item do menu vira um quadro: inserir, buscar, remover, acessos,
    #  imprimir árvore, exportar grafo). Cortamos após a linha de resultado.
    full = [f"$ ./btree demo{m}.bin"] + sess
    RESULT = ("Inserido", "Encontrado", "Não encontrado", "Removido",
              "Acessos", "Gerado", "Essa chave")
    blocks, cur = [], []
    for ln in full:
        cur.append(ln)
        s = ln.strip()
        # resultado de uma ação: fecha o quadro logo após o "Tempo:" seguinte,
        # ou já na própria linha quando não há cronômetro (Acessos/Gerado)
        if any(k in s for k in RESULT) or s.startswith("[RRN"):
            blocks.append(cur[:]); cur = []
    if cur:
        blocks.append(cur[:])

    # agrupa o "[RRN ...]" da impressão da árvore num único quadro
    merged, i = [], 0
    while i < len(blocks):
        b = blocks[i]
        if any(x.strip().startswith("[RRN") for x in b):
            while i + 1 < len(blocks) and \
                    any(x.strip().startswith("[RRN") for x in blocks[i + 1]):
                i += 1; b = b + blocks[i]
        merged.append(b); i += 1

    acc = []
    for i, b in enumerate(merged):
        acc += b
        last = (i == len(merged) - 1)
        push(term_frame(f"{title} — menu interativo", acc),
             2600 if last else 1500)

    # ---- frame final: o grafo gerado ----
    push(graph_frame(f"Grafo gerado — Arvore B de ordem m={m}",
                     "cada caixa = um no em disco (RRN);  abaixo, as chaves do no",
                     graph_png), 3600)

    frames[0].save(os.path.join(OUT, out_name), save_all=True,
                   append_images=frames[1:], duration=durations, loop=0,
                   optimize=True, disposal=2)
    print("GIF:", out_name, "—", len(frames), "frames")
    return frames, durations


def build_poster(session_file, graph_png, out_name, m):
    """Quadro estático (poster) para mídias que não animam GIF (ex.: PDF):
    terminal à esquerda (impressão da árvore) + grafo gerado à direita."""
    sess = read(session_file)
    # pega o trecho com a árvore impressa
    tree = [l for l in sess if l.strip().startswith("[RRN")]
    head = [f"$ ./btree demo{m}.bin", "", f"=== B-Tree (ordem {m}) ===",
            "  ... inserir / buscar / remover ...", "",
            "> Imprimir arvore:"]
    term = term_frame(f"Arvore B (ordem m={m}) — demo", head + tree)
    g = graph_frame(f"Grafo gerado (m={m})",
                    "cada caixa = um no em disco (RRN)", graph_png)
    poster = Image.new("RGB", (W * 2 + 20, H), (255, 255, 255))
    poster.paste(term, (0, 0)); poster.paste(g, (W + 20, 0))
    poster.save(os.path.join(OUT, out_name))
    print("POSTER:", out_name)


def build_graphs_pair(out_name):
    """m3 e m5 lado a lado — os 'grafos gerados' para um slide estático."""
    g3 = Image.open(os.path.join(GA, "m3_demo.png")).convert("RGB")
    g5 = Image.open(os.path.join(GA, "m5_demo.png")).convert("RGB")
    pad, gap, top = 30, 50, 70
    sc = min((g3.width + g5.width), 1) and 1
    th = max(g3.height, g5.height)
    cw = g3.width + g5.width + gap + 2 * pad
    img = Image.new("RGB", (cw, th + top + pad + 40), (255, 255, 255))
    d = ImageDraw.Draw(img)
    d.text((pad, top // 2), "Grafos gerados pelo programa (exportDot + Graphviz)",
           font=SANS_B, fill=(29, 78, 216), anchor="lm")
    img.paste(g3, (pad, top))
    img.paste(g5, (pad + g3.width + gap, top))
    d.text((pad + g3.width // 2, top + th + 18), "ordem m=3  (40 -> 7 chaves)",
           font=SANS, fill=(75, 85, 99), anchor="mm")
    d.text((pad + g3.width + gap + g5.width // 2, top + th + 18),
           "ordem m=5  (mais chaves por no, arvore mais rasa)",
           font=SANS, fill=(75, 85, 99), anchor="mm")
    img.save(os.path.join(OUT, out_name))
    print("PAIR:", out_name)


if __name__ == "__main__":
    f3, d3 = build_gif_for(3, "build_m3.txt", "session_m3.txt",
                           os.path.join(GA, "m3_demo.png"), "demo_m3.gif")
    f5, d5 = build_gif_for(5, "build_m5.txt", "session_m5.txt",
                           os.path.join(GA, "m5_demo.png"), "demo_m5.gif")

    # GIF combinado m3 + m5 (uma única mídia para a apresentação)
    allf, alld = f3 + f5, d3 + d5
    allf[0].save(os.path.join(OUT, "demo_btree.gif"), save_all=True,
                 append_images=allf[1:], duration=alld, loop=0,
                 optimize=True, disposal=2)
    print("GIF: demo_btree.gif —", len(allf), "frames")

    build_poster("session_m5.txt", os.path.join(GA, "m5_demo.png"),
                 "demo_poster.png", 5)
    build_graphs_pair("graphs_m3_m5.png")
    print("\nGIFs/posters em:", OUT)
