#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef ENCRYPT
    #define SERVERTYPE 1
#elif DECRYPT
    #define SERVERTYPE 0
#endif

#define IP_PROTOCOL 0

int main(int argc, char *argv[]) {
    int socketfd, portno, ret;
    struct sockaddr_in serv_addr;
    if (argc != 4) {
        fprintf(stderr, "Usage: %s message key port\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *message_buffer = argv[1];
    char *key_buffer = argv[2];
    size_t message_len = strlen(message_buffer);
    size_t key_len = strlen(key_buffer);
    if (message_len != key_len) {
        fprintf(stderr, "Key and message must be the same length.\n");
        exit(EXIT_FAILURE);
    }
    portno = (int)strtol(argv[3], NULL, 10);
    socketfd = socket(AF_INET, SOCK_STREAM, IP_PROTOCOL);
    if (socketfd < 0) {
        perror("Couldn't open socket!");
        exit(1);
    }
    //bzero((char*) &serv_addr, sizeof(serv_addr));
    inet_pton(AF_INET, argv[2], &(serv_addr.sin_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    ret = connect(socketfd, &serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        perror("Couldn't connect to the socket");
        exit(1);
    }
    ret= write(socketfd, SERVERTYPE, sizeof(char));
    if (ret< 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
    // FIXME: Handle rejection case better!

    ret= write(socketfd, &message_len, sizeof(size_t));
    if (ret< 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
    ret= write(socketfd, message_buffer, message_len);
    if (ret< 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    ret= write(socketfd, key_buffer, message_len);
    if (ret< 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    ret= read(socketfd, message_buffer, message_len);
    if (ret< 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    printf("%s\n", message_buffer);
    close(socketfd);
    return 0;
}
    
