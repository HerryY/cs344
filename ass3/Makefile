CC=clang
CFLAGS=-g -Wpedantic -Wall

all:
	${CC} smallsh.c -o smallsh ${CFLAGS}

docs:
	doxygen smallsh.c

clean:
	rm -rf *.dSYM
	rm -f smallsh
	rm -rf html
