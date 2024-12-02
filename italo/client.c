#include "commom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void log_error_message_invalid_arguments(int argc, char **argv)
{
    printf("Invalid arguments!\n");
    printf("Example IPV4: %s 127.0.0.1 50000 60000 1\n", argv[0]);
    printf("Example IPV6: %s ::1 50000 60000 1\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int socket_response;
    int connect_response;
    char addrstr[BUFFER_SIZE];
    char data_buffer[BUFFER_SIZE];
    int count;

    if (argc < 5)
    {
        log_error_message_invalid_arguments(argc, argv);
    }

    printf("passou do argc\n");

    if (atoi(argv[4]) < 1 || atoi(argv[4]) > 10)
    {
        printf("Invalid argument");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage storage;
    int hasValidParseAddrStorage = addrparse(argv[1], argv[2], &storage);
    if (hasValidParseAddrStorage == -1)
    {
        log_error_message_invalid_arguments(argc, argv);
    }

    // Abertura socket
    socket_response = socket(storage.ss_family, SOCK_STREAM, 0);
    if (socket_response == -1)
    {
        logexit("socket");
    }

    //Abertura Ativa
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    connect_response = connect(socket_response, addr, sizeof(storage));
    if (connect_response != 0)
    {
        logexit("connect");
    }
    printf("passou do connect\n");

    char sendDataBuffer[BUFFER_SIZE];
    char receiveDataBuffer[BUFFER_SIZE];

    // memset(sendDataBuffer, 0, BUFFER_SIZE);
    // snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%s", argv[5]);
    // printf("Antes do send");
    // count = send(socket_response, sendDataBuffer, strlen(sendDataBuffer) + 1, 0);

    // printf("Enviou o primeiro send");

    // if (count != strlen(sendDataBuffer) + 1)
    // {
    //     logexit("send");
    // }

    while (1)
    {
        // // Recebimento de mensagem
        // memset(receiveDataBuffer, 0, BUFFER_SIZE);
        // count = recv(socket_response, receiveDataBuffer, BUFFER_SIZE, 0);
        // if (count == 0)
        // {
        //     // Conexão fechada pelo servidor
        //     printf("Server disconnected\n");
        //     break;
        // }

        // Envio de mensagem
        memset(sendDataBuffer, 0, BUFFER_SIZE);
        printf("mensagem > ");
        fgets(sendDataBuffer, BUFFER_SIZE - 1, stdin);
        count = send(socket_response, sendDataBuffer, strlen(sendDataBuffer) + 1, 0);

        if (count != strlen(sendDataBuffer) + 1)
        {
            logexit("send");
        }

        // Recebimento de mensagem
        memset(receiveDataBuffer, 0, BUFFER_SIZE);
        count = recv(socket_response, receiveDataBuffer, BUFFER_SIZE, 0);
        if (count == 0)
        {
            // Conexão fechada pelo servidor
            printf("Server disconnected\n");
            break;
        }
        puts(receiveDataBuffer);
        puts(sendDataBuffer);
    }

    close(socket_response);
    exit(EXIT_SUCCESS);
}