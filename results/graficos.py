"""Gera graficos PNG a partir de medicoes.csv (ReportLab + Poppler)."""

import csv
import os
import statistics
import subprocess
import tempfile
from pathlib import Path

from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont


PASTA = Path(__file__).resolve().parent
ARQUIVO = PASTA / "medicoes.csv"
LARGURA, ALTURA = 900, 520
ESQUERDA, DIREITA, BASE, TOPO = 100, 850, 100, 430
AZUL = (0.12, 0.35, 0.68)
LARANJA = (0.91, 0.44, 0.10)
CINZA = (0.63, 0.67, 0.72)
pdfmetrics.registerFont(TTFont("ArialEmbedded", "/System/Library/Fonts/Supplemental/Arial.ttf"))


def texto(c, x, y, valor, tamanho=13, cor=(0.14, 0.18, 0.23), alinhar="left"):
    c.setFillColorRGB(*cor)
    c.setFont("ArialEmbedded", tamanho)
    getattr(c, {"left": "drawString", "center": "drawCentredString",
                "right": "drawRightString"}[alinhar])(x, y, valor)


def eixos(c, titulo, maximo, passos, sufixo):
    texto(c, ESQUERDA, 475, titulo, 24)
    for passo in range(passos + 1):
        valor = maximo * passo / passos
        y = BASE + (TOPO - BASE) * passo / passos
        c.setStrokeColorRGB(0.89, 0.91, 0.94)
        c.line(ESQUERDA, y, DIREITA, y)
        texto(c, ESQUERDA - 12, y - 4, f"{valor:g}{sufixo}", 11, alinhar="right")
    c.setStrokeColorRGB(0.20, 0.25, 0.30)
    c.setLineWidth(1.5)
    c.line(ESQUERDA, BASE, DIREITA, BASE)
    c.line(ESQUERDA, BASE, ESQUERDA, TOPO)


def altura(valor, maximo):
    return BASE + (TOPO - BASE) * valor / maximo


def ponto(c, x, y, cor, raio=5):
    c.setFillColorRGB(*cor)
    c.circle(x, y, raio, stroke=0, fill=1)


def linha(c, pontos, cor, espessura=3):
    c.setStrokeColorRGB(*cor)
    c.setLineWidth(espessura)
    for (x1, y1), (x2, y2) in zip(pontos, pontos[1:]):
        c.line(x1, y1, x2, y2)
    for x, y in pontos:
        ponto(c, x, y, cor)


def gerar(nome, desenhar):
    with tempfile.TemporaryDirectory() as temporario:
        pdf = Path(temporario) / "grafico.pdf"
        c = canvas.Canvas(str(pdf), pagesize=(LARGURA, ALTURA))
        desenhar(c)
        c.showPage()
        c.save()
        config = Path(temporario) / "fonts.conf"
        cache = Path(temporario) / "cache"
        cache.mkdir()
        config.write_text("<fontconfig><dir>/System/Library/Fonts/Supplemental</dir>"
                          f"<cachedir>{cache}</cachedir></fontconfig>")
        ambiente = os.environ.copy()
        ambiente["FONTCONFIG_FILE"] = str(config)
        ambiente["XDG_CACHE_HOME"] = str(cache)
        processo = subprocess.run(
            ["pdftoppm", "-f", "1", "-l", "1", "-singlefile", "-png", "-r", "150",
             str(pdf), str(PASTA / nome)],
            env=ambiente, capture_output=True, text=True, timeout=60,
        )
        if processo.returncode:
            raise RuntimeError(processo.stderr[-2000:])


def main():
    with ARQUIVO.open(newline="") as arquivo:
        registros = list(csv.DictReader(arquivo))
    dados = {}
    for trabalhadores in (1, 2, 4, 8):
        tempos = [float(r["tempo_ms"]) for r in registros
                  if int(r["trabalhadores"]) == trabalhadores]
        dados[trabalhadores] = (statistics.median(tempos), min(tempos), max(tempos))
    referencia = dados[1][0]

    def tempo(c):
        eixos(c, "Tempo de execucao", 160, 4, " ms")
        for indice, trabalhadores in enumerate((1, 2, 4, 8)):
            x = 175 + indice * 185
            mediana, minimo, maximo = dados[trabalhadores]
            c.setFillColorRGB(*(CINZA if trabalhadores == 1 else AZUL))
            c.rect(x - 43, BASE, 86, altura(mediana, 160) - BASE, stroke=0, fill=1)
            c.setStrokeColorRGB(0.12, 0.18, 0.24)
            c.setLineWidth(2)
            c.line(x, altura(minimo, 160), x, altura(maximo, 160))
            c.line(x - 9, altura(minimo, 160), x + 9, altura(minimo, 160))
            c.line(x - 9, altura(maximo, 160), x + 9, altura(maximo, 160))
            texto(c, x, altura(maximo, 160) + 13, f"{mediana:.1f}", 12, alinhar="center")
            texto(c, x, BASE - 25, "Sequencial" if trabalhadores == 1 else f"{trabalhadores} threads",
                  13, alinhar="center")
        texto(c, ESQUERDA, 35, "Barras: mediana de 5 repeticoes. Hastes: minimo e maximo.", 12)

    def aceleracao(c):
        eixos(c, "Aceleracao observada", 8, 4, "x")
        xs = [170, 350, 530, 710]
        ideal = [(x, altura(p, 8)) for x, p in zip(xs, (1, 2, 4, 8))]
        observado = [(x, altura(referencia / dados[p][0], 8))
                     for x, p in zip(xs, (1, 2, 4, 8))]
        linha(c, ideal, CINZA, 2)
        linha(c, observado, AZUL)
        for x, p in zip(xs, (1, 2, 4, 8)):
            texto(c, x, BASE - 25, str(p), 13, alinhar="center")
            texto(c, x, altura(referencia / dados[p][0], 8) + 13,
                  f"{referencia / dados[p][0]:.2f}x", 12, alinhar="center")
        texto(c, 730, 360, "Ideal", 12, CINZA)
        texto(c, 730, 340, "Medido", 12, AZUL)
        texto(c, ESQUERDA, 35, "Quantidade de trabalhadores (1 = versao sequencial)", 12)

    def eficiencia(c):
        eixos(c, "Eficiencia paralela", 1.0, 5, "")
        xs = [250, 475, 700]
        c.setStrokeColorRGB(*CINZA)
        c.setLineWidth(2)
        c.line(ESQUERDA, TOPO, DIREITA, TOPO)
        valores = [referencia / dados[p][0] / p for p in (2, 4, 8)]
        linha(c, [(x, altura(valor, 1.0)) for x, valor in zip(xs, valores)], LARANJA)
        for x, p, valor in zip(xs, (2, 4, 8), valores):
            texto(c, x, BASE - 25, f"{p} threads", 13, alinhar="center")
            texto(c, x, altura(valor, 1.0) + 13, f"{valor:.2f}", 12, alinhar="center")
        texto(c, ESQUERDA, 35, "E(p) = T sequencial / (p x T paralelo)", 12)

    gerar("grafico-tempo", tempo)
    gerar("grafico-aceleracao", aceleracao)
    gerar("grafico-eficiencia", eficiencia)


if __name__ == "__main__":
    main()
