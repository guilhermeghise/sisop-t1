# SISOP T1 — Contagem de objetos em matriz binária

Trabalho 1 de Sistemas Operacionais (PUCRS, 2026/II, Prof. Filipo Mór). Duas
versões em ANSI C (C89/C90) contam componentes de células `1` com
**conectividade 8**: uma sequencial e outra paralela com Pthreads.

**Autores:** Guilherme Ghise (23200662) e Eduardo Ferrari (23280274).

## Entrega

| Item | Local |
|---|---|
| Relatório técnico | [`RELATORIO_TECNICO.md`](RELATORIO_TECNICO.md) |
| Slides | [`slides/apresentacao.pdf`](slides/apresentacao.pdf) |
| Vídeo (YouTube, não listado) | <https://youtu.be/2T4hEsPmJSQ> |
| Repositório | <https://github.com/guilhermeghise/sisop-t1> |
| Commit de referência | Registrado no cabeçalho do [relatório técnico](RELATORIO_TECNICO.md) |

## Estrutura do repositório

```text
src/matriz.[ch]                  leitura validada, rotulação e relógio comuns
src/conta-objetos-sequencial.c   versão sequencial
src/conta-objetos-paralelo.c     versão paralela (Pthreads)
tests/obrigatorios/              cinco matrizes do enunciado
tests/adicionais/                casos de borda e matriz de desempenho
tests/check.c                    verificador funcional em ANSI C
results/                         medições brutas (CSV) e gráficos
slides/apresentacao.pdf          apresentação usada no vídeo
Makefile                         compilação (make) e testes (make check)
```

Os executáveis são gerados em `bin/`, que não é versionado.

## Compilar e executar

Requer macOS ou Linux, compilador C com suporte a Pthreads e `make`. O
desenvolvimento e as medições foram feitos no macOS (arm64).

```sh
make
./bin/sequencial tests/obrigatorios/exemplo3.txt
./bin/paralelo tests/obrigatorios/exemplo3.txt 4
```

Saída esperada da versão paralela (o tempo varia conforme a máquina):

```text
Objetos: 5
Trabalhadores: 4
Tempo (ms): 0.072
```

O argumento final da versão paralela é a quantidade de trabalhadores. Se for
maior que o número de linhas, o programa usa uma thread por linha e informa a
quantidade efetiva em `Trabalhadores:`. A versão sequencial imprime apenas
`Objetos:` e `Tempo (ms):`. Para remover os executáveis, use `make clean`.

### Formato de entrada

Cada arquivo começa com `linhas colunas`, seguido de exatamente
`linhas × colunas` valores `0` ou `1`, separados por espaços ou quebras de
linha. Exemplo:

```text
3 3
1 0 0
0 1 0
0 0 1
```

O resultado é **1 objeto**: as três células estão ligadas por diagonais.

Dimensões nulas ou negativas, valores diferentes de `0`/`1`, dados incompletos,
dados extras e quantidade de trabalhadores inválida são rejeitados com uma
mensagem em `stderr` e código de saída diferente de zero.

## Como funciona

- `src/conta-objetos-sequencial.c` percorre toda a matriz e inicia um flood
  fill iterativo para cada componente ainda não rotulado.
- `src/conta-objetos-paralelo.c` divide as linhas em faixas. Cada thread rotula
  apenas sua faixa. Depois de `pthread_join`, a thread principal compara as
  células dos dois lados de cada fronteira (acima à esquerda, acima e acima à
  direita) e une rótulos equivalentes com union-find com compressão de
  caminho. A matriz é somente lida pelas threads; cada uma escreve em posições
  exclusivas do vetor de rótulos, portanto não há necessidade de mutex.
- `src/matriz.c` e `src/matriz.h` contêm a leitura validada e a rotulação comum
  às duas versões. Não há flood fill recursivo.

## Testes e desempenho

```sh
make check
```

`make check` compila um verificador em ANSI C e confere os cinco exemplos
obrigatórios, casos adicionais, 30 matrizes aleatórias com referência
independente em C, entradas inválidas e a matriz de desempenho. Esta última,
versionada em [`tests/adicionais/desempenho.txt`](tests/adicionais/desempenho.txt),
tem 2400 × 2400 células e é usada sem alteração em todas as configurações. As
medições brutas estão em [`results/medicoes.csv`](results/medicoes.csv).

No MacBook Pro M4 usado no trabalho, as medianas de cinco repetições foram:

| Configuração | Mediana (ms) |
|---|---:|
| Sequencial | 137,768 |
| 2 threads | 69,665 |
| 4 threads | 35,709 |
| 8 threads | 19,469 |

Todas as configurações encontraram **176.011 objetos** nessa matriz. Consulte
o [relatório técnico](RELATORIO_TECNICO.md) para método, dispersão, gráficos e
limitações da comparação.

## Versionamento: commit e push

Para obter exatamente a versão avaliada, clone o repositório e faça checkout
do commit de referência indicado no cabeçalho do relatório técnico:

```sh
git clone https://github.com/guilhermeghise/sisop-t1.git
cd sisop-t1
git checkout <hash-do-commit-de-referencia>
make check
```

Fluxo adotado pelo grupo na branch `main`:

1. Atualizar a cópia local antes de alterar arquivos: `git pull --rebase`.
2. Rodar `make check` antes de cada commit que toque código ou testes.
3. Fazer commits pequenos, com mensagem em português no imperativo descrevendo
   a mudança (por exemplo, `Corrige validacao de dimensoes e trabalhadores
   negativos`). Não versionar `bin/` nem arquivos gerados localmente.
4. Enviar com `git push origin main` e conferir no GitHub se o commit chegou.

Como um commit não pode conter o próprio hash, o commit de referência é o
último commit de conteúdo da entrega. Em seguida, um commit separado
atualiza apenas o hash registrado no relatório técnico.

## Referências

1. MÓR, Filipo. *Enunciado do Trabalho 1 e modelo de relatório*. Sistemas
   Operacionais, PUCRS, 2026/II. Fonte dos requisitos e das cinco matrizes
   obrigatórias, transcritas para [`tests/obrigatorios/`](tests/obrigatorios/).
2. THE OPEN GROUP. *The Open Group Base Specifications Issue 7 (POSIX.1-2017)*:
   [`pthread_create`](https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_create.html),
   [`pthread_join`](https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_join.html)
   e [`clock_gettime`](https://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_gettime.html).
3. ROSENFELD, Azriel; PFALTZ, John L. Sequential operations in digital picture
   processing. *Journal of the ACM*, v. 13, n. 4, p. 471–494, 1966.
   Rotulação de componentes conexos em imagens binárias.
4. TARJAN, Robert E. Efficiency of a good but not linear set union algorithm.
   *Journal of the ACM*, v. 22, n. 2, p. 215–225, 1975. Union-find com
   compressão de caminho.

### Ferramentas de apoio

Python 3, ReportLab e Poppler foram usados na preparação dos dados e figuras
já versionados; nenhum script dessas ferramentas integra o código-fonte
entregue. Compilação, testes e execução requerem apenas C/POSIX e `make`.

[Codex](https://openai.com/codex/), [ChatGPT](https://chatgpt.com/) e
[Claude](https://claude.ai/) foram utilizados como ferramentas de apoio ao
desenvolvimento e à documentação. Suas sugestões são submetidas à revisão
do grupo, que permanece responsável pelas decisões de implementação,
pela validação dos resultados e pelo domínio do código. A correção foi
verificada com as matrizes obrigatórias, casos adicionais e uma referência
independente; os resultados de desempenho têm medições brutas versionadas.
