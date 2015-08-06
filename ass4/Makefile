CC=clang
CFLAGS=-Wall -Wall -Wextra -g -std=c99

all: keygen.c

keygen.c:
	${CC} keygen.c -o keygen ${CFLAGS}

clean:
	rm -rf *.dSYM
	rm -f keygen
