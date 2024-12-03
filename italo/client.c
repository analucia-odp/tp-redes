#include "commom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void log_error_message_invalid_arguments()
{
    printf("Invalid arguments!\n");
    printf("Example IPV4: ./client 127.0.0.1 50000 60000 1\n");
    printf("Example IPV6: ./client ::1 50000 60000 1\n");
    exit(EXIT_FAILURE);
}

int initSocketClient(const char *addr, const char *port, struct sockaddr_storage *storage)
{
    int hasValidParseAddrStorage = addrparse(addr, port, storage);
    if (hasValidParseAddrStorage == -1)
    {
        log_error_message_invalid_arguments();
    }

    // Abertura socket
    int socket_response = socket(storage->ss_family, SOCK_STREAM, 0);
    if (socket_response == -1)
    {
        logexit("socket");
    }

    return socket_response;
}

int connectSocketClient(int socket, struct sockaddr_storage *storage)
{
    struct sockaddr *addr = (struct sockaddr *)(storage);
    int connect_response = connect(socket, addr, sizeof(*storage));
    if (connect_response != 0)
    {
        logexit("connect");
    }
    return connect_response;
}

int main(int argc, char **argv)
{
    // if (argc < 5)
    // {
    //     log_error_message_invalid_arguments();
    // }

    if (atoi(argv[4]) < 1 || atoi(argv[4]) > 10)
    {
        printf("Invalid argument");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage storageUserServer;
    struct sockaddr_storage storageLocationServer;

    char *addr = argv[1];
    char *portUserServer = argv[2];
    char *portLocationServer = argv[3];
    char *localizationCode = argv[4];

    // Inicialização Sockets
    int socketUserServer = initSocketClient(addr, portUserServer, &storageUserServer);
    int socketLocationServer = initSocketClient(addr, portLocationServer, &storageLocationServer);

    // Abertura Ativa
    int connectUserServer = connectSocketClient(socketUserServer, &storageUserServer);
    int connectLocationServer = connectSocketClient(socketLocationServer, &storageLocationServer);

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
        int count = send(socketUserServer, sendDataBuffer, strlen(sendDataBuffer) + 1, 0);

        if (count != strlen(sendDataBuffer) + 1)
        {
            logexit("send");
        }

        count = send(socketLocationServer, sendDataBuffer, strlen(sendDataBuffer) + 1, 0);

        if (count != strlen(sendDataBuffer) + 1)
        {
            logexit("send");
        }

        // Recebimento de mensagem
        memset(receiveDataBuffer, 0, BUFFER_SIZE);
        count = recv(socketUserServer, receiveDataBuffer, BUFFER_SIZE, 0);
        count = recv(socketLocationServer, receiveDataBuffer, BUFFER_SIZE, 0);
        if (count == 0)
        {
            // Conexão fechada pelo servidor
            printf("Server disconnected\n");
            break;
        }
        puts(receiveDataBuffer);
        puts(sendDataBuffer);
    }

    close(socketUserServer);
    close(socketLocationServer);
    exit(EXIT_SUCCESS);
}