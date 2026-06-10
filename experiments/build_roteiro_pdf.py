#!/usr/bin/env python3
import os
import re

from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (SimpleDocTemplate, Paragraph, Spacer, Table,
                                TableStyle, HRFlowable)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "apresentacao", "ROTEIRO_APRESENTACAO.md")
DST = os.path.join(ROOT, "apresentacao", "ROTEIRO_APRESENTACAO.pdf")

FD = "/usr/share/fonts/truetype/dejavu"
pdfmetrics.registerFont(TTFont("DJ", f"{FD}/DejaVuSans.ttf"))
pdfmetrics.registerFont(TTFont("DJ-B", f"{FD}/DejaVuSans-Bold.ttf"))
pdfmetrics.registerFont(TTFont("DJ-M", f"{FD}/DejaVuSansMono.ttf"))
pdfmetrics.registerFontFamily("DJ", normal="DJ", bold="DJ-B", italic="DJ",
                              boldItalic="DJ-B")

ACCENT = colors.HexColor("#1d4ed8")
INK = colors.HexColor("#111827")
MUTED = colors.HexColor("#6b7280")

ss = getSampleStyleSheet()
H1 = ParagraphStyle("H1", parent=ss["Heading1"], fontName="DJ-B", fontSize=18,
                    textColor=ACCENT, spaceBefore=4, spaceAfter=8, leading=22)
H2 = ParagraphStyle("H2", parent=ss["Heading2"], fontName="DJ-B", fontSize=13,
                    textColor=ACCENT, spaceBefore=12, spaceAfter=4, leading=16)
H3 = ParagraphStyle("H3", parent=ss["Heading3"], fontName="DJ-B", fontSize=11,
                    textColor=INK, spaceBefore=8, spaceAfter=3, leading=14)
BODY = ParagraphStyle("BODY", fontName="DJ", fontSize=9.5, textColor=INK,
                      leading=13.5, spaceAfter=4)
BULLET = ParagraphStyle("BULLET", parent=BODY, leftIndent=12, bulletIndent=2)
QUOTE = ParagraphStyle("QUOTE", parent=BODY, leftIndent=10, textColor=MUTED,
                       fontSize=9, leading=12.5)

def inline(text):
    text = text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    for emo, rep in (("⏱", ""), ("⭐", " (*)"), ("🎯", ""), ("✅", "[ok]"),
                     ("⚡", ""), ("📊", ""), ("📁", ""), ("🔍", ""),
                     ("🛠️", ""), ("📌", ""), ("🧮", ""), ("🗑️", "")):
        text = text.replace(emo, rep)
    text = re.sub(r"`([^`]+)`", r'<font face="DJ-M" size=8.5>\1</font>', text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", text)
    text = re.sub(r"(?<!\w)_([^_]+)_(?!\w)", r"<i>\1</i>", text)
    return text.strip()

def parse_table(block):
    rows = []
    for ln in block:
        cells = [c.strip() for c in ln.strip().strip("|").split("|")]
        if all(set(c) <= set("-: ") for c in cells):
            continue
        rows.append(cells)
    if not rows:
        return None
    data = [[Paragraph(inline(c), ParagraphStyle("tc", fontName="DJ",
            fontSize=8.5, leading=11, textColor=INK)) for c in r] for r in rows]
    t = Table(data, hAlign="LEFT", repeatRows=1)
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), ACCENT),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
        ("FONTNAME", (0, 0), (-1, 0), "DJ-B"),
        ("GRID", (0, 0), (-1, -1), 0.4, colors.HexColor("#cbd5e1")),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1),
         [colors.white, colors.HexColor("#eef2ff")]),
        ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
        ("TOPPADDING", (0, 0), (-1, -1), 3),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
    ]))
    return t

def build():
    with open(SRC, encoding="utf-8") as f:
        lines = f.read().split("\n")

    flow = []
    buf = []
    kind = None

    def flush():
        nonlocal buf, kind
        if not buf:
            return
        text = inline(" ".join(buf))
        if kind == "bullet":
            flow.append(Paragraph(text, BULLET, bulletText="•"))
        elif kind == "quote":
            flow.append(Paragraph(text, QUOTE))
        else:
            flow.append(Paragraph(text, BODY))
        buf, kind = [], None

    i = 0
    n = len(lines)
    while i < n:
        ln = lines[i]
        s = ln.strip()

        if not s:
            flush(); flow.append(Spacer(1, 4)); i += 1; continue

        if s.startswith("|"):
            flush()
            block = []
            while i < n and lines[i].strip().startswith("|"):
                block.append(lines[i]); i += 1
            t = parse_table(block)
            if t:
                flow.append(Spacer(1, 2)); flow.append(t); flow.append(Spacer(1, 6))
            continue

        if s.startswith("### "):
            flush(); flow.append(Paragraph(inline(s[4:]), H3))
        elif s.startswith("## "):
            flush(); flow.append(Paragraph(inline(s[3:]), H2))
        elif s.startswith("# "):
            flush(); flow.append(Paragraph(inline(s[2:]), H1))
        elif re.match(r"^-{3,}$", s):
            flush()
            flow.append(Spacer(1, 2))
            flow.append(HRFlowable(width="100%", thickness=0.6,
                                   color=colors.HexColor("#cbd5e1")))
            flow.append(Spacer(1, 2))
        elif s.startswith("> "):
            if kind != "quote":
                flush(); kind = "quote"
            buf.append(s[2:])
        elif s.startswith("- ") or s.startswith("• "):
            flush(); kind = "bullet"; buf.append(s[2:])
        elif s.startswith("→"):
            flush(); kind = "quote"; buf.append(s)
        elif s.startswith("**"):
            flush(); kind = "p"; buf.append(s)
        elif re.match(r"^_.*_$", s):
            flush(); flow.append(Paragraph(inline(s), BODY))
        else:
            if not buf:
                kind = "p"
            buf.append(s)
        i += 1

    flush()

    doc = SimpleDocTemplate(DST, pagesize=A4,
                            leftMargin=18 * mm, rightMargin=18 * mm,
                            topMargin=16 * mm, bottomMargin=16 * mm,
                            title="Roteiro de Apresentação — Árvore B")

    def footer(canvas, d):
        canvas.saveState()
        canvas.setFont("DJ", 7.5)
        canvas.setFillColor(MUTED)
        canvas.drawString(18 * mm, 9 * mm,
                          "Roteiro — Árvore B em Memória Secundária (AED-PG-2026)")
        canvas.drawRightString(A4[0] - 18 * mm, 9 * mm, f"{d.page}")
        canvas.restoreState()

    doc.build(flow, onFirstPage=footer, onLaterPages=footer)
    print("PDF:", DST)

if __name__ == "__main__":
    build()
