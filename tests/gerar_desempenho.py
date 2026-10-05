"""Gera a matriz de desempenho reproduzivel (2400 x 2400; semente fixa)."""

import random
from pathlib import Path


def main():
    linhas = colunas = 2400
    gerador = random.Random(20261005)
    destino = Path(__file__).resolve().parent / "adicionais" / "desempenho.txt"
    with destino.open("w") as arquivo:
        arquivo.write(f"{linhas} {colunas}\n")
        for _ in range(linhas):
            arquivo.write(" ".join("1" if gerador.random() < 0.35 else "0"
                                   for _ in range(colunas)) + "\n")
    print(destino)


if __name__ == "__main__":
    main()
