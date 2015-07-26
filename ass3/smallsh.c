#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void runloop();
char* create_file_token(char **word, unsigned int max_length);
void parse_and_run(char *line, unsigned int length);

int main() {
        runloop();
        puts ("out the bottom");
}

void runloop() {
        char *line = NULL;
        size_t linecap = 0;
        ssize_t line_len;
                printf(":");
        while (true) {
                if((line_len = getline(&line, &linecap, stdin)) > 0) {
                        parse_and_run(line, (unsigned int)line_len);
                        printf(":");
                        fflush(stdout);
                }
        }

}

void parse_and_run(char *line, unsigned int length) {
        // command [arg1 arg2 ...] [< input_file] [> output_file] [&]

        // Overcommit --
        // there can never be more arguments than there are characters in the
        // line read from the user.
        char **args = malloc(length * sizeof(char*)); 

        const char *sep = " \n";
        const char devnull[] = "/dev/null/";

        int infile = STDIN_FILENO;
        int outfile = STDOUT_FILENO;
        pid_t pid;
        int pipes[2];

        char *word = NULL;
        char *input = NULL;
        char *output = NULL;
        bool is_background = false;

        unsigned int args_index = 1;
        unsigned int input_len = 0;
        unsigned int output_len = 0;

        char *command = strtok(line, sep);
        if (command == NULL) return;
        args[0] = command;

        do {
                word = strtok(NULL, sep);
                if (word == NULL) {
                        break;
                } else if (strncmp(word, "<", 1) == 0) {
                        word = strtok(NULL, sep);
                        input = create_file_token(&word, length);
                        continue;
                } else if (strncmp(word, ">", 1) == 0) {
                        word = strtok(NULL, sep);
                        output = create_file_token(&word, length);
                        continue;
                } else if (strncmp(word, "&", 1) == 0) {
                        is_background = true;
                        break;
                }
                args[args_index] = word;
                args_index++;
        } while (word != NULL);

        if (is_background && input == NULL) {
                // redirect to /dev/null
                input = (char*)devnull;
        }
        if (is_background && output == NULL) {
                // redirect to /dev/null
                output = (char*)devnull;
        }

        pid = fork();
        if (pid == 0) { // child
                if (input != NULL) {
                        infile = open(input, O_RDONLY);
                        dup2(infile, STDIN_FILENO);
                        close(infile);
                }
                if (output != NULL) {
                        outfile = open(output, O_WRONLY | O_CREAT);
                        dup2(outfile, STDOUT_FILENO);
                        close(outfile);
                }

                execvp(command, args);
                printf("%s: command not found\n", command);
                exit(0);
        } else { // parent
                int child_status;
                if (!is_background)
                        waitpid(pid, &child_status, 0);
        }
        if (input != devnull && input != NULL) {
                puts(input);
                free(input);
        }
        if (output != devnull && output != NULL) {
                free(output);
        }
        free(args);
}

char* create_file_token(char **word, unsigned int max_length) {
        size_t token_length = strnlen(*word, max_length);
        char *token = malloc(token_length * sizeof(char*));
        strncpy(token, *word, token_length);
        return token;
}
