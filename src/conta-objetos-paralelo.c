#include "matriz.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void *executar_regiao(void *argumento)
{
    Regiao *regiao;
    regiao = (Regiao *)argumento;
    regiao->erro = rotular_regiao(regiao);
    return NULL;
}

static size_t raiz(size_t *pais, size_t indice)
{
    /* Encurta o caminho ate a raiz para acelerar as proximas consultas. */
    while (pais[indice] != indice) {
        pais[indice] = pais[pais[indice]];
        indice = pais[indice];
    }
    return indice;
}

static int unir(size_t *pais, size_t a, size_t b)
{
    size_t raiz_a, raiz_b;
    raiz_a = raiz(pais, a);
    raiz_b = raiz(pais, b);
    /* So uma uniao nova reduz a contagem; contatos repetidos nao descontam. */
    if (raiz_a == raiz_b)
        return 0;
    pais[raiz_a] = raiz_b;
    return 1;
}

static int unir_celulas(size_t *pais, const size_t *rotulos, size_t a, size_t b)
{
    if (rotulos[a] == 0 || rotulos[b] == 0)
        return 0;
    return unir(pais, rotulos[a] - 1, rotulos[b] - 1);
}

int main(int argc, char **argv)
{
    Matriz matriz;
    struct timespec inicio, fim;
    pthread_t *threads;
    Regiao *regioes;
    size_t *rotulos, *pais, total, trabalhadores, iniciados, i, linha, coluna;
    size_t topo, baixo, componentes, base, sobra;
    unsigned long solicitado;
    char *fim_numero;
    int erro, retorno, estado;

    if (argc != 3) {
        fprintf(stderr, "Uso: %s matriz.txt trabalhadores\n", argv[0]);
        return EXIT_FAILURE;
    }
    errno = 0;
    solicitado = strtoul(argv[2], &fim_numero, 10);
    if (argv[2][0] == '-' || fim_numero == argv[2] || *fim_numero != '\0' ||
        errno == ERANGE || solicitado == 0 || (size_t)solicitado != solicitado) {
        fprintf(stderr, "Quantidade de trabalhadores invalida\n");
        return EXIT_FAILURE;
    }
    if (ler_matriz(argv[1], &matriz) != 0)
        return EXIT_FAILURE;
    trabalhadores = (size_t)solicitado;
    if (trabalhadores > matriz.linhas)
        trabalhadores = matriz.linhas;
    total = matriz.linhas * matriz.colunas;
    if (total > ((size_t)-1) / sizeof(size_t) ||
        trabalhadores > ((size_t)-1) / sizeof(pthread_t) ||
        trabalhadores > ((size_t)-1) / sizeof(Regiao) ||
        clock_gettime(CLOCK_MONOTONIC, &inicio) != 0) {
        fprintf(stderr, "Matriz grande demais ou relogio indisponivel\n");
        liberar_matriz(&matriz);
        return EXIT_FAILURE;
    }
    rotulos = (size_t *)calloc(total, sizeof(size_t));
    pais = (size_t *)malloc(total * sizeof(size_t));
    regioes = (Regiao *)calloc(trabalhadores, sizeof(Regiao));
    threads = (pthread_t *)malloc(trabalhadores * sizeof(pthread_t));
    if (rotulos == NULL || pais == NULL || regioes == NULL || threads == NULL) {
        fprintf(stderr, "Memoria insuficiente\n");
        free(rotulos);
        free(pais);
        free(regioes);
        free(threads);
        liberar_matriz(&matriz);
        return EXIT_FAILURE;
    }

    /* As primeiras faixas recebem uma linha extra quando a divisao tem sobra. */
    base = matriz.linhas / trabalhadores;
    sobra = matriz.linhas % trabalhadores;
    iniciados = 0;
    erro = 0;
    for (i = 0; i < trabalhadores; ++i) {
        regioes[i].matriz = &matriz;
        regioes[i].inicio = i * base + (i < sobra ? i : sobra);
        regioes[i].fim = regioes[i].inicio + base + (i < sobra ? 1 : 0);
        regioes[i].rotulos = rotulos;
        regioes[i].pais = pais;
        /* Cada thread escreve apenas em sua faixa; a matriz e somente leitura. */
        retorno = pthread_create(&threads[i], NULL, executar_regiao, &regioes[i]);
        if (retorno != 0) {
            fprintf(stderr, "pthread_create falhou: %d\n", retorno);
            erro = 1;
            break;
        }
        ++iniciados;
    }
    /* Aguarda todas as escritas, inclusive se a criacao falhou parcialmente. */
    for (i = 0; i < iniciados; ++i) {
        retorno = pthread_join(threads[i], NULL);
        if (retorno != 0) {
            fprintf(stderr, "pthread_join falhou: %d\n", retorno);
            return EXIT_FAILURE;
        }
        if (regioes[i].erro != 0)
            erro = 1;
    }
    if (erro) {
        fprintf(stderr, "Falha no processamento paralelo\n");
        estado = EXIT_FAILURE;
        goto limpar;
    }

    componentes = 0;
    for (i = 0; i < trabalhadores; ++i)
        componentes += regioes[i].componentes;

    /* Apos os joins, une fronteiras em serie: acima e as duas diagonais. */
    for (i = 1; i < trabalhadores; ++i) {
        linha = regioes[i].inicio;
        for (coluna = 0; coluna < matriz.colunas; ++coluna) {
            topo = (linha - 1) * matriz.colunas + coluna;
            baixo = linha * matriz.colunas + coluna;
            if (coluna > 0)
                componentes -= (size_t)unir_celulas(pais, rotulos, baixo, topo - 1);
            componentes -= (size_t)unir_celulas(pais, rotulos, baixo, topo);
            if (coluna + 1 < matriz.colunas)
                componentes -= (size_t)unir_celulas(pais, rotulos, baixo, topo + 1);
        }
    }
    if (clock_gettime(CLOCK_MONOTONIC, &fim) != 0) {
        perror("clock_gettime");
        estado = EXIT_FAILURE;
        goto limpar;
    }
    printf("Objetos: %lu\nTrabalhadores: %lu\nTempo (ms): %.3f\n",
           (unsigned long)componentes, (unsigned long)trabalhadores,
           tempo_ms(&inicio, &fim));
    estado = EXIT_SUCCESS;

limpar:
    free(rotulos);
    free(pais);
    free(regioes);
    free(threads);
    liberar_matriz(&matriz);
    return estado;
}
