#ifndef MATRIZ_H
#define MATRIZ_H

#include <stddef.h>
#include <time.h>

typedef struct {
    size_t linhas;
    size_t colunas;
    unsigned char *celulas;
} Matriz;

typedef struct {
    const Matriz *matriz;
    size_t inicio;
    size_t fim;
    size_t *rotulos;
    size_t *pais;
    size_t componentes;
    int erro;
} Regiao;

int ler_matriz(const char *caminho, Matriz *matriz);
void liberar_matriz(Matriz *matriz);
int rotular_regiao(Regiao *regiao);
double tempo_ms(const struct timespec *inicio, const struct timespec *fim);

#endif
