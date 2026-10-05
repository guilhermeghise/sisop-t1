"""Verifica os cinco exemplos do enunciado e casos de fronteira."""

import random
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CASOS = {
    "tests/obrigatorios/exemplo1.txt": 3,
    "tests/obrigatorios/exemplo2.txt": 4,
    "tests/obrigatorios/exemplo3.txt": 5,
    "tests/obrigatorios/exemplo4.txt": 6,
    "tests/obrigatorios/exemplo5.txt": 7,
    "tests/adicionais/zeros.txt": 0,
    "tests/adicionais/diagonal.txt": 1,
    "tests/adicionais/um-objeto.txt": 1,
    "tests/adicionais/uma-linha.txt": 3,
}


def executar(programa, arquivo, trabalhadores=None):
    comando = [str(ROOT / "bin" / programa), str(arquivo)]
    if trabalhadores is not None:
        comando.append(str(trabalhadores))
    saida = subprocess.run(comando, check=True, capture_output=True, text=True).stdout
    return int(next(linha.split()[1] for linha in saida.splitlines()
                    if linha.startswith("Objetos:")))


def referencia(matriz):
    linhas, colunas = len(matriz), len(matriz[0])
    visitados = set()
    objetos = 0
    for linha in range(linhas):
        for coluna in range(colunas):
            if matriz[linha][coluna] == 0 or (linha, coluna) in visitados:
                continue
            objetos += 1
            pilha = [(linha, coluna)]
            visitados.add((linha, coluna))
            while pilha:
                atual_linha, atual_coluna = pilha.pop()
                for viz_linha in range(max(0, atual_linha - 1), min(linhas, atual_linha + 2)):
                    for viz_coluna in range(max(0, atual_coluna - 1), min(colunas, atual_coluna + 2)):
                        vizinho = (viz_linha, viz_coluna)
                        if matriz[viz_linha][viz_coluna] and vizinho not in visitados:
                            visitados.add(vizinho)
                            pilha.append(vizinho)
    return objetos


def conferir(arquivo, esperado):
    assert executar("sequencial", arquivo) == esperado, arquivo
    for trabalhadores in (2, 3, 4, 8):
        assert executar("paralelo", arquivo, trabalhadores) == esperado, (arquivo, trabalhadores)


def main():
    for caminho, esperado in CASOS.items():
        conferir(ROOT / caminho, esperado)

    gerador = random.Random(20261005)
    with tempfile.TemporaryDirectory() as pasta:
        arquivo = Path(pasta) / "matriz.txt"
        for _ in range(30):
            linhas = gerador.randrange(2, 13)
            colunas = gerador.randrange(1, 13)
            matriz = [[gerador.randrange(2) for _ in range(colunas)] for _ in range(linhas)]
            arquivo.write_text(
                f"{linhas} {colunas}\n" +
                "\n".join(" ".join(map(str, linha)) for linha in matriz) + "\n"
            )
            conferir(arquivo, referencia(matriz))

        for conteudo in ("0 3\n", "2 2\n1 0\n", "1 1\n2\n", "1 1\n1\n0\n"):
            arquivo.write_text(conteudo)
            for programa, args in (("sequencial", []), ("paralelo", ["2"])):
                processo = subprocess.run(
                    [str(ROOT / "bin" / programa), str(arquivo), *args],
                    capture_output=True,
                )
                assert processo.returncode != 0, (programa, conteudo)

    for _ in range(10):
        assert executar("paralelo", ROOT / "tests/obrigatorios/exemplo3.txt", 4) == 5
    print("OK: 5 matrizes obrigatorias, 4 casos adicionais, 30 matrizes aleatorias e entradas invalidas")


if __name__ == "__main__":
    main()
