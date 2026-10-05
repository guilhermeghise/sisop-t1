#include "matriz.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv)
{
    Matriz matriz;
    Regiao regiao;
    struct timespec inicio, fim;
    size_t total, *rotulos;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s matriz.txt\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (ler_matriz(argv[1], &matriz) != 0)
        return EXIT_FAILURE;
    total = matriz.linhas * matriz.colunas;
    if (total > ((size_t)-1) / sizeof(size_t) ||
        clock_gettime(CLOCK_MONOTONIC, &inicio) != 0) {
        fprintf(stderr, "Matriz grande demais ou relogio indisponivel\n");
        liberar_matriz(&matriz);
        return EXIT_FAILURE;
    }
    rotulos = (size_t *)calloc(total, sizeof(size_t));
    if (rotulos == NULL) {
        fprintf(stderr, "Memoria insuficiente para rotulos\n");
        liberar_matriz(&matriz);
        return EXIT_FAILURE;
    }
    regiao.matriz = &matriz;
    regiao.inicio = 0;
    regiao.fim = matriz.linhas;
    regiao.rotulos = rotulos;
    regiao.pais = NULL;
    if (rotular_regiao(&regiao) != 0 || clock_gettime(CLOCK_MONOTONIC, &fim) != 0) {
        fprintf(stderr, "Falha na contagem ou no relogio\n");
        free(rotulos);
        liberar_matriz(&matriz);
        return EXIT_FAILURE;
    }
    printf("Objetos: %lu\nTempo (ms): %.3f\n",
           (unsigned long)regiao.componentes, tempo_ms(&inicio, &fim));
    free(rotulos);
    liberar_matriz(&matriz);
    return EXIT_SUCCESS;
}
