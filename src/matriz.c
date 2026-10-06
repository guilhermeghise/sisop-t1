#include "matriz.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int ler_dimensao(FILE *arquivo, size_t *dimensao)
{
    char texto[64], *fim;
    unsigned long valor;
    int separador;

    if (fscanf(arquivo, "%63s", texto) != 1 || texto[0] == '-')
        return -1;
    /* O limite de leitura nao pode dividir um numero entre duas dimensoes. */
    separador = fgetc(arquivo);
    if (separador != EOF && !isspace((unsigned char)separador))
        return -1;
    errno = 0;
    valor = strtoul(texto, &fim, 10);
    if (errno == ERANGE || fim == texto || *fim != '\0' ||
        valor == 0 || (size_t)valor != valor)
        return -1;
    *dimensao = (size_t)valor;
    return 0;
}

int ler_matriz(const char *caminho, Matriz *matriz)
{
    FILE *arquivo;
    size_t linhas, colunas, total, i;
    char texto[8];
    char sobra;

    matriz->linhas = 0;
    matriz->colunas = 0;
    matriz->celulas = NULL;
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        perror(caminho);
        return -1;
    }
    /* Confere o produto antes de calcular o tamanho da alocacao. */
    if (ler_dimensao(arquivo, &linhas) != 0 ||
        ler_dimensao(arquivo, &colunas) != 0 ||
        linhas > ((size_t)-1) / colunas) {
        fprintf(stderr, "Dimensoes invalidas em %s\n", caminho);
        fclose(arquivo);
        return -1;
    }
    total = linhas * colunas;
    matriz->celulas = (unsigned char *)malloc(total);
    if (matriz->celulas == NULL) {
        fprintf(stderr, "Memoria insuficiente para a matriz\n");
        fclose(arquivo);
        return -1;
    }
    for (i = 0; i < total; ++i) {
        if (fscanf(arquivo, "%7s", texto) != 1 ||
            (strcmp(texto, "0") != 0 && strcmp(texto, "1") != 0)) {
            fprintf(stderr, "Matriz invalida em %s (celula %lu)\n",
                    caminho, (unsigned long)i);
            liberar_matriz(matriz);
            fclose(arquivo);
            return -1;
        }
        matriz->celulas[i] = (unsigned char)(texto[0] - '0');
    }
    if (fscanf(arquivo, " %c", &sobra) == 1 || ferror(arquivo)) {
        fprintf(stderr, "Dados extras ou erro de leitura em %s\n", caminho);
        liberar_matriz(matriz);
        fclose(arquivo);
        return -1;
    }
    if (fclose(arquivo) != 0) {
        perror(caminho);
        liberar_matriz(matriz);
        return -1;
    }
    matriz->linhas = linhas;
    matriz->colunas = colunas;
    return 0;
}

void liberar_matriz(Matriz *matriz)
{
    free(matriz->celulas);
    matriz->celulas = NULL;
    matriz->linhas = 0;
    matriz->colunas = 0;
}

int rotular_regiao(Regiao *regiao)
{
    const Matriz *matriz;
    size_t capacidade, *fila;
    size_t linha, coluna, indice, cabeca, cauda, atual, atual_linha, atual_coluna;
    size_t viz_linha, viz_coluna, lin_min, lin_max, col_min, col_max, vizinho;
    size_t rotulo;

    matriz = regiao->matriz;
    regiao->componentes = 0;
    capacidade = (regiao->fim - regiao->inicio) * matriz->colunas;
    if (capacidade == 0 || capacidade > ((size_t)-1) / sizeof(size_t))
        return -1;
    /* A fila no heap evita recursao e comporta todas as celulas da faixa. */
    fila = (size_t *)malloc(capacidade * sizeof(size_t));
    if (fila == NULL)
        return -1;

    for (linha = regiao->inicio; linha < regiao->fim; ++linha) {
        for (coluna = 0; coluna < matriz->colunas; ++coluna) {
            indice = linha * matriz->colunas + coluna;
            if (matriz->celulas[indice] == 0 || regiao->rotulos[indice] != 0)
                continue;
            /* Indice global + 1 distingue faixas e reserva zero para nao visitado. */
            rotulo = indice + 1;
            regiao->rotulos[indice] = rotulo;
            if (regiao->pais != NULL)
                regiao->pais[indice] = indice;
            ++regiao->componentes;
            cabeca = 0;
            cauda = 0;
            fila[cauda++] = indice;
            while (cabeca < cauda) {
                atual = fila[cabeca++];
                atual_linha = atual / matriz->colunas;
                atual_coluna = atual % matriz->colunas;
                /* O flood fill nao atravessa a faixa atribuida a esta thread. */
                lin_min = atual_linha > regiao->inicio ? atual_linha - 1 : atual_linha;
                lin_max = atual_linha + 1 < regiao->fim ? atual_linha + 1 : atual_linha;
                col_min = atual_coluna > 0 ? atual_coluna - 1 : atual_coluna;
                col_max = atual_coluna + 1 < matriz->colunas ? atual_coluna + 1 : atual_coluna;
                for (viz_linha = lin_min; viz_linha <= lin_max; ++viz_linha) {
                    for (viz_coluna = col_min; viz_coluna <= col_max; ++viz_coluna) {
                        vizinho = viz_linha * matriz->colunas + viz_coluna;
                        if (matriz->celulas[vizinho] != 0 &&
                            regiao->rotulos[vizinho] == 0) {
                            regiao->rotulos[vizinho] = rotulo;
                            fila[cauda++] = vizinho;
                        }
                    }
                }
            }
        }
    }
    free(fila);
    return 0;
}

double tempo_ms(const struct timespec *inicio, const struct timespec *fim)
{
    return (double)(fim->tv_sec - inicio->tv_sec) * 1000.0 +
           (double)(fim->tv_nsec - inicio->tv_nsec) / 1000000.0;
}
