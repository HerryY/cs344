#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef ENCRYPT
#define SERVERTYPE 'e'
#elif DECRYPT
#define SERVERTYPE 'd'
#endif

#define IP_PROTOCOL 0

void read_message_and_key(char *port, char *message_file, char *key_file);
void send_message(char *port, char *message_buffer, char *key_buffer,
        size_t message_len);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s message key port\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    read_message_and_key(argv[3], argv[1], argv[2]);
    return 0;
}

void read_message_and_key(char *port, char *message_file, char *key_file) {
    int err;
    struct stat buf;
    err = stat(message_file, &buf);
    if (err == -1) {
        perror("Reading message file");
        exit(EXIT_FAILURE);
    }
    // drop newline and EOF
    size_t message_len = buf.st_size - 1;
    err = stat(key_file, &buf);
    if (err == -1) {
        perror("Reading message file");
        exit(EXIT_FAILURE);
    }
    // drop newline and EOF
    size_t key_len = buf.st_size - 1;
    if (key_len < message_len) {
        fprintf(stderr, "Key length and message length differ!\n");
        exit(EXIT_FAILURE);
    }
    char *message_buffer = malloc(message_len);
    char *key_buffer = malloc(message_len);
    int keyfd = open(key_file, O_RDONLY);
    if (keyfd == -1) {
        perror("Error opening key file");
        exit(EXIT_FAILURE);
    }
    int messagefd = open(message_file, O_RDONLY);
    if (messagefd == -1) {
        perror("Error opening message file");
        exit(EXIT_FAILURE);
    }
    err = read(messagefd, message_buffer, message_len);
    if (err == -1) {
        perror("Error reading message file");
        exit(EXIT_FAILURE);
    }
    err = read(keyfd, key_buffer, message_len);
    if (err == -1) {
        perror("Error reading message file");
        exit(EXIT_FAILURE);
    }
    close(keyfd);
    close(messagefd);
    for (size_t i = 0; i < message_len; i++) {
        if (!(message_buffer[i] == ' ' || (message_buffer[i] >= 'A' &&
                        message_buffer[i] <= 'Z'))) {
            fprintf(stderr, "Invalid message character %c.\n",
                    message_buffer[i]);
            printf("%s", message_buffer);
            exit(EXIT_FAILURE);
        }
        if (!(key_buffer[i] == ' ' || (key_buffer[i] >= 'A' &&
                        key_buffer[i] <= 'Z'))) {
            fprintf(stderr, "Invalid key character %c.\n",
                    key_buffer[i]);
            exit(EXIT_FAILURE);
        }
    }

    send_message(port, message_buffer, key_buffer, message_len);
    free(message_buffer);
    free(key_buffer);
}

void send_message(char *port, char *message_buffer, char *key_buffer,
        size_t message_len) {
    int portno = (int)strtol(port, NULL, 10);
    int socketfd, err;
    struct sockaddr_in serv_addr;
    char client_type = SERVERTYPE;
    char server_type = 2;
    socketfd = socket(AF_INET, SOCK_STREAM, IP_PROTOCOL);
    if (socketfd < 0) {
        perror("Couldn't open socket!");
        exit(EXIT_FAILURE);
    }
    bzero((char*) &serv_addr, sizeof(serv_addr));
    inet_pton(AF_INET, port, &(serv_addr.sin_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    err = connect(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (err < 0) {
        perror("Couldn't connect to the socket");
        exit(EXIT_FAILURE);
    }
    err = write(socketfd, &client_type, sizeof(char));
    if (err < 0) {
        perror("Writing server type to socket");
        exit(EXIT_FAILURE);
    }

    err = read(socketfd, &server_type, sizeof(char));
    if (err < 0) {
        perror("Reading server type from socket");
        exit(EXIT_FAILURE);
    }

    if (server_type != client_type) {
        fprintf(stderr, "Can't connect to the wrong "
                "type of server\n");
        shutdown(socketfd, 2);
        close(socketfd);
        exit(EXIT_FAILURE);
    }
    err = write(socketfd, &message_len, sizeof(size_t));
    if (err < 0) {
        perror("Writing message length to socket");
        exit(EXIT_FAILURE);
    }
    err = write(socketfd, message_buffer, message_len);
    if (err < 0) {
        perror("Writing message to socket");
        exit(EXIT_FAILURE);
    }
    /*
       err = write(STDERR_FILENO, message_buffer, message_len);
       write(STDERR_FILENO, "\n", 1);
     */

    err = write(socketfd, key_buffer, message_len);
    if (err < 0) {
        perror("Writing key to socket");
        exit(EXIT_FAILURE);
    }

    err = read(socketfd, message_buffer, message_len);
    if (err < 0) {
        perror("Reading response from socket");
        exit(EXIT_FAILURE);
    }
    write(STDOUT_FILENO, message_buffer, message_len);
    write(STDOUT_FILENO, "\n", 1);
    close(socketfd);
}
