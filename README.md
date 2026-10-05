# SISOP T1 — Contagem de objetos em matriz binária

Trabalho 1 de Sistemas Operacionais (PUCRS, 2026/II). Duas versões em ANSI C
(C89/C90) contam componentes de células `1` com **conectividade 8**: uma
sequencial e outra paralela com Pthreads.

**Autores:** Guilherme Ghise (23200662) e Eduardo Ferrari (23280274).

## Compilar e executar

Requer macOS ou Linux, compilador C com suporte a Pthreads e `make`.

```sh
make
./bin/sequencial tests/obrigatorios/exemplo3.txt
./bin/paralelo tests/obrigatorios/exemplo3.txt 4
```

O argumento final da versão paralela é a quantidade de trabalhadores. Se for
maior que o número de linhas, o programa usa uma thread por linha e informa a
quantidade efetiva. As duas versões imprimem a contagem e o tempo de cálculo em
milissegundos.

Cada arquivo de entrada começa com `linhas colunas`, seguido dos valores `0` e
`1`, separados por espaços ou quebras de linha. Exemplo:

```text
3 3
1 0 0
0 1 0
0 0 1
```

O resultado é **1 objeto**: as três células estão ligadas por diagonais.

## Como funciona

- `src/conta-objetos-sequencial.c` percorre toda a matriz e inicia um flood
  fill iterativo para cada componente ainda não rotulado.
- `src/conta-objetos-paralelo.c` divide as linhas em faixas. Cada thread rotula
  apenas sua faixa. Depois de `pthread_join`, a thread principal compara as
  células dos dois lados de cada fronteira (acima à esquerda, acima e acima à
  direita) e une rótulos equivalentes com union-find. A matriz é somente lida
  pelas threads; cada uma escreve em posições exclusivas.
- `src/matriz.c` e `src/matriz.h` contêm a leitura validada e a rotulação comum
  às duas versões. Não há flood fill recursivo.

## Testes e desempenho

```sh
make check
```

`make check` compila um verificador em ANSI C e confere os cinco exemplos
obrigatórios, casos adicionais, 30 matrizes aleatórias com referência
independente em C, entradas inválidas e a matriz de desempenho. Esta última,
versionada em
[`tests/adicionais/desempenho.txt`](tests/adicionais/desempenho.txt) tem 2400 ×
2400 células e é usada sem alteração em todas as configurações. As medições brutas
estão em [`results/medicoes.csv`](results/medicoes.csv).

No MacBook Pro M4 usado no trabalho, as medianas de cinco repetições foram
137,768 ms (sequencial), 69,665 ms (2 threads), 35,709 ms (4) e 19,469 ms (8).
Todas as configurações encontraram **176.011 objetos** nessa matriz. Consulte
o [relatório técnico](RELATORIO_TECNICO.md) para método, dispersão, gráficos e
limitações da comparação.

## Entrega e referências

- [Relatório técnico](RELATORIO_TECNICO.md)
- [Slides da apresentação](slides/apresentacao.pdf)
- Vídeo da apresentação: (NAO CONCLUIDO)

As cinco matrizes obrigatórias foram transcritas do enunciado do Prof. Filipo
Mór. Os resultados foram conferidos com os testes e as medições versionadas.
Python 3, ReportLab e Poppler foram usados na preparação dos dados e figuras
já versionados; nenhum script dessas ferramentas integra o código-fonte
entregue. Compilação, testes e execução requerem apenas C/POSIX e `make`.
