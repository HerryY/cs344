#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define IP_PROTOCOL 0

#ifdef ENCRYPT
#define SERVERTYPE 'e'
#define CRYPT(a, b) (a) = (int)((int)(a) + (int)(b));
#elif DECRYPT
#define SERVERTYPE 'd'
#define CRYPT(a, b) (a) = (int)((int)(a) - (int)(b));
#endif

void setup(int portno);
void do_otp(size_t message_len, char *key_buffer, char*message_buffer);
void serve_loop(int socketfd);
void cleanup(int clientsockfd, char *key_buffer, char *message_buffer);
void exit_server(int _);
void child_ended(int _signum);


int main (int argc, char *argv[]) {
    int portno;
    signal(SIGINT, exit_server);
    signal(SIGCHLD, child_ended);
    if (argc != 2) {
        fprintf(stderr, "No port provided!\n");
        exit(EXIT_FAILURE);
    }
    portno = (int)strtol(argv[1], NULL, 10);
    setup(portno);
}

// We interrupt traps must have the signature int func(int), but we ignore the
// signal number. Tell clang to suppress unused parameter warnings.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
// Wait for children when receiving SIGINT.
void exit_server(int _) {
    int _child_info;
    pid_t pid;
    while ((pid = waitpid(0, &_child_info, 0)) != -1) {}
    if (errno == ECHILD) {
        exit(EXIT_SUCCESS);
    } else {
        perror("Reaping children");
        exit(EXIT_FAILURE);
    }
}
#pragma clang diagnostic pop

// We interrupt traps must have the signature int func(int), but we ignore the
// signal number. Tell clang to suppress unused parameter warnings.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
// Reap children as they die and server receives SIGCHLD.
void child_ended(int _signum) {
    int _child_info;
    pid_t pid = waitpid(0, &_child_info, 0);
    if (pid == -1) {
        perror("Reaping child after SIGCHLD.");
        exit(EXIT_FAILURE);
    }
}
#pragma clang diagnostic pop

void setup(int portno) {
    int socketfd, err;
    struct sockaddr_in serv_addr;

    // Open a socket over the internet using TCP and the IP protocol
    socketfd = socket(AF_INET, SOCK_STREAM, IP_PROTOCOL);
    if (socketfd < 0) {
        perror("Couldn't open socket!");
        exit(EXIT_FAILURE);
    }
    // Set the address family
    serv_addr.sin_family = AF_INET;
    // Convert from host byte-order to network byte-order
    serv_addr.sin_port = htons(portno);
    // INADDR_ANY is localhost?
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    err = bind(socketfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (err < 0) {
        perror("Failed to bind to the socket");
        exit(EXIT_FAILURE);
    }

    // Apparently the max backlog on most systems is 5
    err = listen(socketfd, 5);
    if (err < 0) {
        perror("Failed to listen on the socket");
        exit(EXIT_FAILURE);
    }
    serve_loop(socketfd);
}

void serve_loop(int socketfd) {
    int newsockfd, err;
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    char client_type = 1;
    char server_type = SERVERTYPE;
    char *message_buffer, *key_buffer;
    size_t message_len;

    while (true) {
        // Accept new connections from the client and fork off workers.
        newsockfd = accept(socketfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("Failed to accept connection");
        }
        pid_t pid = fork();
        if (pid == 0) {  // child
            // Read a single character representing the client type.
            err = read(newsockfd, &client_type, sizeof(char));
            if (err < 0) {
                perror("Failed to read from socket");
                cleanup(newsockfd, NULL, NULL);
                exit(EXIT_FAILURE);
            }

            // Write a single character representing the server type.
            err = write(newsockfd, &server_type, sizeof(char));
            if (err < 0) {
                perror("Failed to write program type to socket");
                cleanup(newsockfd, NULL, NULL);
                exit(EXIT_FAILURE);
            }

            // If the `client_type` is not the same as the server type,
            // that is if there is a decrypt client connecting to an encrypt
            // server or vice versa, then fail to connect.
            if (client_type != server_type) {
                fprintf(stderr, "Rejecting connection from the wrong "
                        "type of client\n");
                shutdown(newsockfd, 2);
                close(newsockfd);
                shutdown(socketfd, 2);
                close(socketfd);
                exit(EXIT_FAILURE);
            }

            // Read one size_t worth of data representing the length of the
            // message.
            err = read(newsockfd, &message_len, sizeof(size_t));
            if (err < 0) {
                perror("Failed to read from socket");
                cleanup(newsockfd, NULL, NULL);
                exit(EXIT_FAILURE);
            }

            // Allocate space for the message and the key. They will be the
            // same size.
            message_buffer = malloc(message_len);
            key_buffer = malloc(message_len);

            // Read the message.
            err = read(newsockfd, message_buffer, message_len);
            if (err < 0) {
                perror("Failed to read from socket");
                cleanup(newsockfd, key_buffer, message_buffer);
                exit(EXIT_FAILURE);
            }

            // Read the key.
            err = read(newsockfd, key_buffer, message_len);
            if (err < 0) {
                perror("Failed to read from socket");
                cleanup(newsockfd, key_buffer, message_buffer);
                exit(EXIT_FAILURE);
            }
            // Perform the actual encryption, writing the result back into the
            // message buffer.
            do_otp(message_len, key_buffer, message_buffer);
#if DEBUG
            //write(STDOUT_FILENO, message_buffer, message_len);
            //printf("\n");
#endif
            // Write the response back to the client.
            err = write(newsockfd, message_buffer, message_len);
            if (err < 0) {
                perror("Failed writing to socket");
            }

            // Clean up.
            cleanup(newsockfd, key_buffer, message_buffer);
            exit(EXIT_SUCCESS);
        } else {
            continue;
        }
    }
}

void do_otp(size_t message_len, char *key_buffer, char *message_buffer) {

#ifdef DEBUG
    write(STDERR_FILENO, message_buffer, message_len);
    write(STDERR_FILENO, "\n", 1);
#endif
    // Replace spaces with '[', which is one more than 'Z'.
    // Then subtract A so that characters which were 'A' are now 0 and
    // characters which were '[' are now 26.
    for (size_t i = 0; i < message_len; i++) {
        if (message_buffer[i] == ' ') {
            message_buffer[i] = '[';
        }
        if (key_buffer[i] == ' ') {
            key_buffer[i] = '[';
        }
        message_buffer[i] = message_buffer[i] - 'A';
        key_buffer[i] = key_buffer[i] - 'A';
    }
    // Actual algorithm. Handle negative modularization.
    for (size_t i = 0; i < message_len; i++) {
        CRYPT(message_buffer[i], key_buffer[i])
            if (message_buffer[i] < 0) {
                message_buffer[i] = 27 + message_buffer[i];
            } else {
                message_buffer[i] %= 27;
            }
    }
    // Undo initial transformation so now we are back in the original alphabet
    // of [A-Z_]
    for (size_t i = 0; i < message_len; i++) {
        message_buffer[i] = message_buffer[i] + 'A';
        key_buffer[i] = key_buffer[i] + 'A';
        if (message_buffer[i] == '[') {
            message_buffer[i] = ' ';
        }
        if (key_buffer[i] == '[') {
            key_buffer[i] = ' ' ;
        }
    }

#ifdef DEBUG
    write(STDERR_FILENO, message_buffer, message_len);
    write(STDERR_FILENO, "\n", 1);
#endif
}

// Shut down the client socket connections and close all file descriptors.
// Also free any buffers which are not NULL.
void cleanup(int clientsockfd, char *key_buffer, char *message_buffer) {
    int err;
    if (message_buffer != NULL) {
        free(message_buffer);
    }
    if (key_buffer != NULL) {
        free(key_buffer);
    }
    err = shutdown(clientsockfd, 2);
    if (err == -1) {
        perror("Shutting down the client socket file descriptor failed");
    }
    close(clientsockfd);
}
