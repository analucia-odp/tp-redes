#include "commom.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

// man socket
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

void log_error_message_invalid_arguments(char **argv)
{
    printf("Invalid arguments!\n");
    printf("Example: %s 40000 50000\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{

    if (argc < 3)
    {
        log_error_message_invalid_arguments(argv);
    }

    struct sockaddr_storage storage;
    int serverInit = buildStorageServer(argv[1], argv[2], &storage);

    if (serverInit == -1)
    {
        printf("Erro na inicialização do servidor!\n");
    }

    // Socket
    int socket_response = socket(storage.ss_family, SOCK_STREAM, 0);
    if (socket_response == -1)
    {
        logexit("socket");
    }

    int enable = 1;
    int setsockopt_result = setsockopt(socket_response, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (setsockopt_result == -1)
    {
        logexit("setsockopt");
    }

    // --------------- Abertura Passiva ------------

    // Bind
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    int bind_response = bind(socket_response, addr, sizeof(storage));
    if (bind_response == -1)
    {
        logexit("bind");
    }

    // Listen
    int listen_response = listen(socket_response, 10); // quantidade de conexões que podem estar pendentes para tratamento
    if (listen_response == -1)
    {
        logexit("listen");
    }

    char addresString[BUFFER_SIZE];

    // parseAddressToString(addr, addresString, BUFFER_SIZE);
    // printf("\nServidor executando na porta: %s", addresString);
    printf("\nEsperando por novas conexões...");
    struct sockaddr_storage clientStorage;

    char sendBufferDataClient[BUFFER_SIZE];
    char receiveBufferDataClient[BUFFER_SIZE];

    while (1)
    {
        struct sockaddr *clientAddress = (struct sockaddr *)(&clientStorage);
        socklen_t clientAddressLen = sizeof(clientStorage);
        int acceptConnectionSocketClient = accept(socket_response, clientAddress, &clientAddressLen);
        if (acceptConnectionSocketClient == -1)
        {
            logexit("accept");
        }
        char clientAddresString[BUFFER_SIZE];
        parseAddressToString(clientAddress, clientAddresString, BUFFER_SIZE);
        printf("Conexão Aceita na porta %s\n", clientAddresString);

        while (1)
        {
            // Recebe mensagem do cliente
            memset(receiveBufferDataClient, 0, BUFFER_SIZE);

            size_t count = recv(acceptConnectionSocketClient, receiveBufferDataClient, BUFFER_SIZE, 0);

            if (count == 0) {
                // Conexão fechada pelo cliente
                printf("Client disconnected\n");
                break;
            }

            // Envia mensagem para o client
            memset(sendBufferDataClient, 0, BUFFER_SIZE);
            sprintf(sendBufferDataClient, "Mensagem recebida");

            size_t bytes_count_send = send(acceptConnectionSocketClient, receiveBufferDataClient, strlen(receiveBufferDataClient) + 1, 0);
            if (bytes_count_send != strlen(sendBufferDataClient) + 1)
                logexit("send");

            // if (strcmp(receiveBufferDataClient, "kill"))
            // {
            //     strcpy(sendBufferDataClient, "Succesful disconnect");
            //     send(acceptConnectionSocketClient, sendBufferDataClient, strlen(sendBufferDataClient) + 1, 0);
            //     close(acceptConnectionSocketClient);
            // }
            // else
            // {
            //     send(acceptConnectionSocketClient, receiveBufferDataClient, strlen(receiveBufferDataClient) + 1, 0);
            // }
        }
        close(acceptConnectionSocketClient);
    }

    close(socket_response);
    exit(EXIT_SUCCESS);
}