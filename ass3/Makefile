CC=clang
CFLAGS=-g -Wpedantic -Wall -std=c99

all:
	${CC} smallsh.c -o smallsh ${CFLAGS}

docs:
	doxygen smallsh.c

clean:
	rm -rf *.dSYM
	rm -f smallsh
	rm -rf html
	rm -rf testdir*
