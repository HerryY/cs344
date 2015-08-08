#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

const char USAGE[] = "Usage: %s number\n\tnumber is the key size in bytes\n";

int main(int argc, char** argv) {
    // If the number of arguments is invalid print a message to stderr and
    // exit with a failure code.
    if (argc != 2) {
        fprintf(stderr, USAGE, argv[0]);
        exit(EXIT_FAILURE);
    }
    // Attempt to convert the `number` argument to a long.
    long key_size = strtol(argv[1], NULL, 10);
    // If the input was out of range, print an error and exit
    if (errno == ERANGE) {
        fprintf(stderr,
                "The input number %s was too large or small.\n"
                "It must be between %ld and %ld\n", 
                argv[0], LONG_MIN, LONG_MAX);
        exit(ERANGE);
    }
    // If the input was invalid, print an error and exit
    if (errno == EINVAL) {
        fprintf(stderr,
                "The input %s was invalid..\n",
                argv[1]);
        exit(EINVAL);
    }
    // Seed the prng with the current time.
    srand(time(0));
    // Print `key_size` many random characters which make up the key.
    for (long i = 0; i < key_size; i++) {
        int num = rand() % 27;
        if (num == 26) {
            num = ' ';
        } else {
            num += 'A';
        }
        printf("%c", (char)num);
    }
    printf("\n");
    return EXIT_SUCCESS;
}
