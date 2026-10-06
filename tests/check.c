#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    const char *arquivo;
    unsigned long esperado;
} Caso;

static int executar(const char *programa, const char *arquivo,
                    int trabalhadores, unsigned long *objetos)
{
    char comando[512];
    FILE *saida;
    int leu, estado;

    if (trabalhadores == 0)
        sprintf(comando, "./bin/%s %s 2>/dev/null", programa, arquivo);
    else
        sprintf(comando, "./bin/%s %s %d 2>/dev/null", programa, arquivo,
                trabalhadores);
    saida = popen(comando, "r");
    if (saida == NULL)
        return -1;
    leu = fscanf(saida, "Objetos: %lu", objetos);
    estado = pclose(saida);
    return leu == 1 && estado == 0 ? 0 : -1;
}

static int conferir(const char *arquivo, unsigned long esperado)
{
    static const int quantidades[] = {2, 3, 4, 8};
    unsigned long objetos;
    size_t i;

    if (executar("sequencial", arquivo, 0, &objetos) != 0 ||
        objetos != esperado) {
        fprintf(stderr, "Falha no sequencial: %s\n", arquivo);
        return -1;
    }
    for (i = 0; i < sizeof(quantidades) / sizeof(quantidades[0]); ++i) {
        if (executar("paralelo", arquivo, quantidades[i], &objetos) != 0 ||
            objetos != esperado) {
            fprintf(stderr, "Falha no paralelo (%d): %s\n",
                    quantidades[i], arquivo);
            return -1;
        }
    }
    return 0;
}

static unsigned long referencia(const unsigned char matriz[12][12],
                                int linhas, int colunas)
{
    unsigned char visitado[12][12];
    int fila[144];
    int linha, coluna, cabeca, cauda, atual, viz_linha, viz_coluna;
    unsigned long objetos;

    memset(visitado, 0, sizeof(visitado));
    objetos = 0;
    for (linha = 0; linha < linhas; ++linha) {
        for (coluna = 0; coluna < colunas; ++coluna) {
            if (matriz[linha][coluna] == 0 || visitado[linha][coluna])
                continue;
            ++objetos;
            cabeca = 0;
            cauda = 0;
            fila[cauda++] = linha * colunas + coluna;
            visitado[linha][coluna] = 1;
            while (cabeca < cauda) {
                atual = fila[cabeca++];
                for (viz_linha = atual / colunas - 1;
                     viz_linha <= atual / colunas + 1; ++viz_linha) {
                    for (viz_coluna = atual % colunas - 1;
                         viz_coluna <= atual % colunas + 1; ++viz_coluna) {
                        if (viz_linha >= 0 && viz_linha < linhas &&
                            viz_coluna >= 0 && viz_coluna < colunas &&
                            matriz[viz_linha][viz_coluna] != 0 &&
                            !visitado[viz_linha][viz_coluna]) {
                            visitado[viz_linha][viz_coluna] = 1;
                            fila[cauda++] = viz_linha * colunas + viz_coluna;
                        }
                    }
                }
            }
        }
    }
    return objetos;
}

static int gravar_matriz(const char *arquivo,
                         const unsigned char matriz[12][12],
                         int linhas, int colunas)
{
    FILE *saida;
    int linha, coluna, erro;

    saida = fopen(arquivo, "w");
    if (saida == NULL)
        return -1;
    erro = fprintf(saida, "%d %d\n", linhas, colunas) < 0;
    for (linha = 0; linha < linhas; ++linha) {
        for (coluna = 0; coluna < colunas; ++coluna)
            erro |= fprintf(saida, "%u%c", (unsigned int)matriz[linha][coluna],
                            coluna + 1 == colunas ? '\n' : ' ') < 0;
    }
    if (fclose(saida) != 0)
        erro = 1;
    return erro ? -1 : 0;
}

static int gravar_texto(const char *arquivo, const char *conteudo)
{
    FILE *saida;
    int erro;

    saida = fopen(arquivo, "w");
    if (saida == NULL)
        return -1;
    erro = fputs(conteudo, saida) == EOF;
    if (fclose(saida) != 0)
        erro = 1;
    return erro ? -1 : 0;
}

int main(void)
{
    static const Caso casos[] = {
        {"tests/obrigatorios/exemplo1.txt", 3},
        {"tests/obrigatorios/exemplo2.txt", 4},
        {"tests/obrigatorios/exemplo3.txt", 5},
        {"tests/obrigatorios/exemplo4.txt", 6},
        {"tests/obrigatorios/exemplo5.txt", 7},
        {"tests/adicionais/zeros.txt", 0},
        {"tests/adicionais/diagonal.txt", 1},
        {"tests/adicionais/um-objeto.txt", 1},
        {"tests/adicionais/uma-linha.txt", 3},
        {"tests/adicionais/desempenho.txt", 176011}
    };
    static const char *invalidos[] = {
        "0 3\n", "2 2\n1 0\n", "1 1\n2\n", "1 1\n1\n0\n",
        "0000000000000000000000000000000"
        "0000000000000000000000000000000" "11\n1\n",
        "1 " "0000000000000000000000000000000"
        "0000000000000000000000000000000" "11\n"
    };
    unsigned char matriz[12][12];
    char temporario[] = "/tmp/sisop-t1-check-XXXXXX";
    unsigned long objetos;
    size_t i;
    int fd, linha, coluna, linhas, colunas, ok;

    for (i = 0; i < sizeof(casos) / sizeof(casos[0]); ++i) {
        if (conferir(casos[i].arquivo, casos[i].esperado) != 0)
            return EXIT_FAILURE;
    }

    fd = mkstemp(temporario);
    if (fd < 0) {
        perror("mkstemp");
        return EXIT_FAILURE;
    }
    if (close(fd) != 0) {
        perror("close");
        unlink(temporario);
        return EXIT_FAILURE;
    }
    ok = 1;
    srand(20261005U);
    for (i = 0; i < 30; ++i) {
        linhas = 2 + rand() % 11;
        colunas = 1 + rand() % 12;
        for (linha = 0; linha < linhas; ++linha)
            for (coluna = 0; coluna < colunas; ++coluna)
                matriz[linha][coluna] = (unsigned char)(rand() % 2);
        if (gravar_matriz(temporario, matriz, linhas, colunas) != 0 ||
            conferir(temporario, referencia(matriz, linhas, colunas)) != 0) {
            ok = 0;
            break;
        }
    }
    for (i = 0; ok && i < sizeof(invalidos) / sizeof(invalidos[0]); ++i) {
        if (gravar_texto(temporario, invalidos[i]) != 0 ||
            executar("sequencial", temporario, 0, &objetos) == 0 ||
            executar("paralelo", temporario, 2, &objetos) == 0) {
            fprintf(stderr, "Entrada invalida aceita: caso %lu\n",
                    (unsigned long)i + 1);
            ok = 0;
        }
    }
    /* Um numero de 63 caracteres completo continua sendo uma dimensao valida. */
    if (ok && (gravar_texto(temporario,
            "0000000000000000000000000000000"
            "0000000000000000000000000000000" "1 1\n1\n") != 0 ||
            conferir(temporario, 1) != 0))
        ok = 0;
    if (ok && (system("./bin/paralelo tests/obrigatorios/exemplo1.txt ' -1' >/dev/null 2>&1") == 0 ||
               system("./bin/paralelo tests/obrigatorios/exemplo1.txt '\t-2' >/dev/null 2>&1") == 0)) {
        fprintf(stderr, "Quantidade negativa de trabalhadores aceita\n");
        ok = 0;
    }
    if (unlink(temporario) != 0) {
        perror("unlink");
        ok = 0;
    }
    for (i = 0; ok && i < 10; ++i) {
        if (executar("paralelo", "tests/obrigatorios/exemplo3.txt", 4,
                     &objetos) != 0 || objetos != 5) {
            fprintf(stderr, "Falha na repeticao do exemplo 3\n");
            ok = 0;
        }
    }
    if (!ok)
        return EXIT_FAILURE;
    puts("OK: 5 matrizes obrigatorias, 4 casos adicionais, 30 matrizes aleatorias e entradas invalidas");
    return EXIT_SUCCESS;
}
