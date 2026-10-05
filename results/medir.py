"""Mede a mesma matriz nas duas versoes e grava as repeticoes brutas."""

import csv
import re
import statistics
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MATRIZ = ROOT / "tests/adicionais/desempenho.txt"
SAIDA = ROOT / "results/medicoes.csv"
CONFIGURACOES = [("sequencial", 1), ("paralela", 2), ("paralela", 4), ("paralela", 8)]


def executar(versao, trabalhadores):
    programa = ROOT / "bin" / ("sequencial" if versao == "sequencial" else "paralelo")
    comando = [str(programa), str(MATRIZ)]
    if versao == "paralela":
        comando.append(str(trabalhadores))
    saida = subprocess.run(comando, check=True, capture_output=True, text=True).stdout
    objetos = int(re.search(r"^Objetos: (\d+)$", saida, re.M).group(1))
    tempo = float(re.search(r"^Tempo \(ms\): ([\d.]+)$", saida, re.M).group(1))
    return objetos, tempo


def main():
    if not MATRIZ.exists():
        raise SystemExit("Execute python3 tests/gerar_desempenho.py antes.")
    referencia, _ = executar("sequencial", 1)
    for _ in range(2):
        for versao, trabalhadores in CONFIGURACOES:
            assert executar(versao, trabalhadores)[0] == referencia

    registros = []
    for repeticao in range(1, 6):
        for versao, trabalhadores in CONFIGURACOES:
            objetos, tempo = executar(versao, trabalhadores)
            if objetos != referencia:
                raise RuntimeError(f"Resultado divergente: {versao} {trabalhadores}")
            registros.append({
                "matriz": "desempenho", "linhas": 2400, "colunas": 2400,
                "versao": versao, "trabalhadores": trabalhadores,
                "repeticao": repeticao, "tempo_ms": f"{tempo:.3f}",
                "objetos": objetos, "resultado_correto": "true",
            })
    with SAIDA.open("w", newline="") as arquivo:
        escritor = csv.DictWriter(arquivo, fieldnames=registros[0].keys(),
                                  lineterminator="\n")
        escritor.writeheader()
        escritor.writerows(registros)
    for versao, trabalhadores in CONFIGURACOES:
        tempos = [float(r["tempo_ms"]) for r in registros
                  if r["versao"] == versao and r["trabalhadores"] == trabalhadores]
        print(versao, trabalhadores, "mediana", statistics.median(tempos),
              "min-max", min(tempos), max(tempos), "objetos", referencia)


if __name__ == "__main__":
    main()
