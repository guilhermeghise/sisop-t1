CC = cc
CFLAGS = -O2 -std=c89 -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=200809L

.PHONY: all check clean

all: bin/sequencial bin/paralelo

bin:
	mkdir -p bin

bin/sequencial: src/conta-objetos-sequencial.c src/matriz.c src/matriz.h | bin
	$(CC) $(CFLAGS) src/conta-objetos-sequencial.c src/matriz.c -o $@

bin/paralelo: src/conta-objetos-paralelo.c src/matriz.c src/matriz.h | bin
	$(CC) $(CFLAGS) -pthread src/conta-objetos-paralelo.c src/matriz.c -o $@

bin/check: tests/check.c | bin
	$(CC) $(CFLAGS) tests/check.c -o $@

check: all bin/check
	./bin/check

clean:
	rm -rf bin
