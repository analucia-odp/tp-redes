#include "commom.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

// man socket
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

void log_error_message_invalid_arguments(char **argv)
{
    printf("Invalid arguments!\n");
    printf("Example IPV4: %s 127.0.0.1 50000 60000 1\n", argv[0]);
    printf("Example IPV6: %s ::1 50000 60000 1\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc < 5)
    {
        log_error_message_invalid_arguments(argv);
    }

    struct sockaddr_storage storage;
    int hasValidParseAddrStorage = buildStorage(argv[1], argv[2], &storage);

    if (hasValidParseAddrStorage == -1)
    {
        log_error_message_invalid_arguments(argv);
    }

    // Abertura socket
    int socket_response = socket(storage.ss_family, SOCK_STREAM, 0);
    if (socket_response == -1)
    {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    printf("Family: %d", addr->sa_family);
    // printf("\nData: %s", addr->sa_data);

    // Abertura Ativa
    int connect_response = connect(socket_response, addr, sizeof(storage));
    if (connect_response == -1)
    {
        logexit("connect");
    }

    char addrStr[BUFFER_SIZE];
    parseAddressToString(addr, addrStr, BUFFER_SIZE);
    printf("Connect to %s\n", addrStr);

    while (1)
    {
        // Envio de mensagem
        char sendDataBuffer[BUFFER_SIZE];
        memset(sendDataBuffer, 0, BUFFER_SIZE);
        printf("> ");
        fgets(sendDataBuffer, BUFFER_SIZE - 1, stdin);

        if (strcmp(sendDataBuffer, "kill") == 0)
        {
            break;
        }

        int countSendMessage = send(socket_response, sendDataBuffer, strlen(sendDataBuffer) + 1, 0);
        if (countSendMessage != strlen(sendDataBuffer) + 1)
        {
            logexit("send");
        }

        // Recebimento de mensagem
        char receiveDataBuffer[BUFFER_SIZE];
        memset(receiveDataBuffer, 0, BUFFER_SIZE);
        int countReceive = recv(socket_response, receiveDataBuffer, BUFFER_SIZE, 0);
    }

    close(socket_response);
    exit(EXIT_SUCCESS);
}