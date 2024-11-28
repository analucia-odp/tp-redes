#include "commom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void usage(int argc, char **argv)
{
    printf("usage: %s <serverIP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int socket_response;
    int connect_response;
    char addrstr[BUFFER_SIZE];
    char data_buffer[BUFFER_SIZE];
    int count;

    if (argc < 3)
    {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage))
    {
        usage(argc, argv);
    }

    socket_response = socket(storage.ss_family, SOCK_STREAM, 0);
    if (socket_response == -1)
    {
        logexit("socket");
    }
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    connect_response = connect(socket_response, addr, sizeof(storage));
    if (connect_response != 0)
    {
        logexit("connect");
    }

    while (1)
    {
        memset(data_buffer, 0, BUFFER_SIZE);
        printf("mensagem > ");
        fgets(data_buffer, BUFFER_SIZE - 1, stdin);
        count = send(socket_response, data_buffer, strlen(data_buffer) + 1, 0);

        if (count != strlen(data_buffer) + 1)
        {
            logexit("send");
        }

        memset(data_buffer, 0, BUFFER_SIZE);
        count = recv(socket_response, data_buffer, BUFFER_SIZE, 0);
        if (count == 0)
        {
            // Conexão fechada pelo servidor
            printf("Server disconnected\n");
            break;
        }
        puts(data_buffer);
    }

    close(socket_response);
    exit(EXIT_SUCCESS);
}