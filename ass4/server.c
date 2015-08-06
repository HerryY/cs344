#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define IP_PROTOCOL 0

#ifdef ENCRYPT
    #define SERVERTYPE 1
#elif DECRYPT
    #define SERVERTYPE 0
#endif

// FIXME wait on children when closing.

void setup(int portno);
void do_otp(size_t message_len, char *key_buffer, char*message_buffer);
void cleanup(int serversockfd, int clientsockfd);
void serve_loop(int socketfd);


int main (int argc, char * argv[]) {
    int portno;
    if (argc < 2) {
        fprintf(stderr, "No port provided!\n");
        exit(EXIT_FAILURE);
    }
    portno = (int)strtol(argv[1], NULL, 10);
}

void setup(portno) {
    int socketfd, err;
    struct sockaddr_in serv_addr;

    // Open a socket over the internet using TCP and the IP protocol
    socketfd = socket(AF_INET, SOCK_STREAM, IP_PROTOCOL);
    if (socketfd < 0) {
        perror("Couldn't open socket!");
        exit(EXIT_FAILURE);
    }
    // FIXME: Is this necessary?
    //bzero((char*) &serv_addr, sizeof(serv_addr));
    // Set the address family
    serv_addr.sin_family = AF_INET;
    // Convert from host byte-order to network byte-order
    serv_addr.sin_port = htons(portno);
    // FIXME: Answer this question
    // Apparently INADDR_ANY is localhost?
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
    socklen_t clilen = sizeof(cli_addr);
    char client_type = '\0';
    char *message_buffer, *key_buffer;
    size_t message_len;
    struct sockaddr_in cli_addr;

    while (true) {
        // Accept new connections from the client and fork off workers.
        // FIXME: rename newsockfd
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
                cleanup(socketfd, newsockfd, NULL, NULL);
                exit(EXIT_FAILURE);
            }
            // If the `client_type` is not the same as the server type,
            // that is if there is a decrypt client connecting to an encrypt
            // server or vice versa, then fail to connect.
            if (client_type != SERVERTYPE) {
                fprintf(stderr, "Rejecting connection from the wrong "
                        "type of client\n");
                cleanup(socketfd, newsockfd);
                exit(EXIT_FAILURE);
            }

            // Read one size_t worth of data representing the length of the
            // message.
            err = read(newsockfd, &message_len, sizeof(size_t));
            if (err < 0) {
                perror("Failed to read from socket");
                cleanup(socketfd, newsockfd, NULL, NULL);
                exit(EXIT_FAILURE);
            }

            // Allocate space for the message and the key. They will be the
            //same size.
            message_buffer = malloc(message_len);
            key_buffer = malloc(message_len);

            // Read the message.
            err = read(newsockfd, message_buffer, message_len);
            if (err < 0) {
                perror("Failed to read from socket");
                cleanup(socketfd, newsockfd, key_buffer, message_buffer);
                exit(EXIT_FAILURE);
            }

            // Read the key.
            err = read(newsockfd, key_buffer, message_len);
            if (err < 0) {
                perror("Failed to read from socket");
                cleanup(socketfd, newsockfd, key_buffer, message_buffer);
                exit(EXIT_FAILURE);
            }
            // Perform the actual encryption, writing the result back into the
            // message buffer.
            do_otp(message_len, key_buffer, message_buffer);
            #if DEBUG
                write(STDOUT_FILENO, message_buffer, message_len);
                printf("\n");
            #endif
            // Write the response back to the client.
            err = write(newsockfd, message_buffer, message_len);
            if (err < 0) {
                perror("Failed writing to socket");
            }

            // Clean up.
            cleanup(socketfd, newsockfd, key_buffer, message_buffer);
            exit(EXIT_SUCCESS);
        } else {
            continue;
        }
    }
}

void do_otp(size_t message_len, char *key_buffer, char*message_buffer) {
    // FIXME: this is just a dummy for now
    memcpy(key_buffer, message_buffer, message_len);
}

// Shut down the client socket connections and close all file descriptors.
// Also free any buffers which are not NULL.
void cleanup(int serversockfd, int clientsockfd, char *key_buffer,
             char *message_buffer) {
    if (message_buffer != NULL) {
        free(message_buffer);
    }
    if (key_buffer != NULL) {
        free(key_buffer);
    }
    int err = shutdown(clientsockfd, 2);
    if (err == -1) {
        perror("Shutting down the client socket file descriptor failed");
    }
    err = shutdown(serversockfd, 2);
    if (err == -1) {
        perror("Shutting down the server socket file descriptor failed");
    }
    close(clientsockfd);
    close(serversockfd);
}
