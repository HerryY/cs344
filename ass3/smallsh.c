#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct child_list {
        unsigned int num;
        unsigned int cap;
        pid_t *children;
} bg_child_list;
int shell_status = 0;


void runloop();
char* create_file_token(char **word, unsigned int max_length);
void parse_and_run(char *line, unsigned int length);
bool word_has_comment(char *word);
void trap_interrupt(int signum);
void print_args(char **arr);



void init_child_list();
void destroy_child_list();
void push_child_list(pid_t child);
pid_t pop_child_list();

void init_child_list() {
        bg_child_list.num = 0;
        bg_child_list.cap = 4;
        bg_child_list.children = malloc(bg_child_list.cap * sizeof(pid_t));
}

void destroy_child_list() {
        free(bg_child_list.children);
}

void push_child_list(pid_t child) {
        if (bg_child_list.num == bg_child_list.cap) {
                bg_child_list.cap *= 2;
                bg_child_list.children = realloc(bg_child_list.children,
                                                 bg_child_list.cap *
                                                 sizeof(pid_t));
        }
        bg_child_list.children[bg_child_list.num] = child;
        bg_child_list.num++;
}

pid_t pop_child_list() {
        if (bg_child_list.num > 0) {
                bg_child_list.num--;
                return bg_child_list.children[bg_child_list.num + 1];
        } else {
                return 0;
        }
}

int main() {
        init_child_list();
        signal(SIGINT, trap_interrupt);
        runloop();
        return 0;
}

void trap_interrupt(int signum) {
        pid_t child;
        while ((child = pop_child_list())) {
                kill(child, SIGKILL);
        }
}

void runloop() {
        char *line = NULL;
        size_t linecap = 0;
        ssize_t line_len;
                printf(":");
        while (true) {
                if((line_len = getline(&line, &linecap, stdin)) > 0) {
                        //fprintf(stderr, "%s\n",line);
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
        const char devnull[] = "/dev/null";

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

        if (strcmp(command, "exit") == 0) {
                trap_interrupt(-1);
                exit(1);
        } else if (strcmp(command, "cd") == 0) {
                char *dest = strtok(NULL, sep);
                if (dest == NULL) {
                        dest = "";
                }
                if(chdir(dest) == -1) {
                        printf("%s: No such file or directory\n", dest);
                        return;
                }
                return;
        } else if (strcmp(command, "status") == 0) {
                if (WIFEXITED(shell_status))
                        printf("Exited: %d\n", WEXITSTATUS(shell_status));
                if (WIFSIGNALED(shell_status))
                        printf("Stop signal: %d\n", WSTOPSIG(shell_status));
                if (WTERMSIG(shell_status))
                        printf("Termination signal: %d\n",
                               WTERMSIG(shell_status));
                return;
        }


        do {
                word = strtok(NULL, sep);
                if (word == NULL) {
                        break;
                } else if (word_has_comment(word)) {
                        args[args_index] = word;
                        args_index++;
                        break;
                } else if (strncmp(word, "<", 1) == 0) {
                        word = strtok(NULL, sep);
                        input = create_file_token(&word, length);
                        fflush(stdout);
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
        args[args_index] = NULL;

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
                        if (infile != -1) {
                                perror("infile");
                        }
                        dup2(infile, STDIN_FILENO);
                        close(infile);
                }
                if (output != NULL) {
                        outfile = open(output, O_WRONLY | O_CREAT);
                        if (outfile != -1) {
                                perror("infile");
                        }
                        dup2(outfile, STDOUT_FILENO);
                        close(outfile);
                }

                execvp(command, args);
                perror("command not found");
                exit(1);
        } else { // parent
                if (!is_background) {
                        waitpid(pid, &shell_status, 0);
                } else {
                        push_child_list(pid);
                }
        }
        if (input != devnull && input != NULL) {
                free(input);
        }
        if (output != devnull && output != NULL) {
                free(output);
        }
        free(args);
}

void print_args(char **arr) {
        for (int i = 0; arr[i] != NULL; i++) {
                printf("%s, ", arr[i]);
        }
        printf("\n");

}

bool word_has_comment(char *word) {
        for (int i = 0; word[i] != '\0'; i++) {
                if (word[i] == '#') {
                        word[i] = '\0';
                        return true;
                }
        }
        return false;
}

char* create_file_token(char **word, unsigned int max_length) {
        size_t token_length = strnlen(*word, max_length);
        char *token = malloc(token_length * sizeof(char*));
        strncpy(token, *word, token_length);
        return token;
}
