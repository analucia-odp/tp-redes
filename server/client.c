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

void sendMessage(int socket, char *message)
{
    int count = send(socket, message, strlen(message) + 1, 0);
    if (count != strlen(message) + 1)
    {
        logexit("send");
    }
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

void receiveMessage(int socket, char *buffer)
{
    memset(buffer, 0, BUFFER_SIZE);
    int count = recv(socket, buffer, BUFFER_SIZE, 0);
    if (count == 0)
    {
        // Conexão fechada pelo servidor
        printf("Server disconnected\n");
    }
}

int main(int argc, char **argv)
{

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

    // Abertura de comunicação de Cliente com Servidores
    memset(sendDataBuffer, 0, BUFFER_SIZE);
    snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%d %s", REQ_CONN, localizationCode);

    // Envia pedido de comunicação
    sendMessage(socketUserServer, sendDataBuffer);
    sendMessage(socketLocationServer, sendDataBuffer);

    short int clientId;

    // Recebe resposta do servidor de usuario
    receiveMessage(socketUserServer, receiveDataBuffer);
    char str[20];
    sprintf(str, "%d", RES_CONN);
    short int clientUserId;
    short int locationUserId;

    if (strstr(receiveDataBuffer, str) != NULL)
    {
        sscanf(receiveDataBuffer, "21 %hd", &clientId);
        printf("SU New ID: %hd\n", clientId);
        clientUserId = clientId;
    }
    // Recebe resposta do servidor de localização
    receiveMessage(socketLocationServer, receiveDataBuffer);
    if (strstr(receiveDataBuffer, str) != NULL)
    {
        sscanf(receiveDataBuffer, "21 %hd", &clientId);
        printf("SL New ID: %hd\n", clientId);
        locationUserId = clientId;
    }
    while (1)
    {
        // Envio de mensagem
        memset(sendDataBuffer, 0, BUFFER_SIZE);
        printf("mensagem > ");
        fgets(sendDataBuffer, BUFFER_SIZE - 1, stdin);

        // Fechamento de conexão
        if (strcmp(sendDataBuffer, "kill\n") == 0)
        {
            // SEND REQ_DISC
            memset(sendDataBuffer, 0, BUFFER_SIZE);
            snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%d %hd", REQ_DISC, clientUserId);
            sendMessage(socketUserServer, sendDataBuffer);
            memset(sendDataBuffer, 0, BUFFER_SIZE);
            snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%d %hd", REQ_DISC, locationUserId);
            sendMessage(socketLocationServer, sendDataBuffer);

            // RCV REQ_DISC
            receiveMessage(socketUserServer, receiveDataBuffer);
            // printf("mensagem recebida %s\n", receiveDataBuffer);
            if (strstr(receiveDataBuffer, "0") != NULL)
            {
                printf("SU Successful disconnect\n");
            }
            else if (strstr(receiveDataBuffer, "255") != NULL)
            {
                printf("%s\n", receiveDataBuffer);
            }
            receiveMessage(socketLocationServer, receiveDataBuffer);
            if (strstr(receiveDataBuffer, "0") != NULL)
            {
                printf("SL Successful disconnect\n");
            }
            else if (strstr(receiveDataBuffer, "255") != NULL)
            {
                printf("%s\n", receiveDataBuffer);
            }
            break;
        }

        // Adiciona usuário
        if (strstr(sendDataBuffer, "add") != NULL)
        {
            char uuid[10];
            int isEspecial;
            sscanf(sendDataBuffer, "add %s %d", uuid, &isEspecial);
            memset(sendDataBuffer, 0, BUFFER_SIZE);
            snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%d %s %d", REQ_USRADD, uuid, isEspecial);
            sendMessage(socketUserServer, sendDataBuffer);
            receiveMessage(socketUserServer, receiveDataBuffer);

            char str[20];
            sprintf(str, "%d", OK);
            if (strstr(receiveDataBuffer, str) != NULL)
            {
                char buffer[BUFFER_SIZE];
                char buffer2[BUFFER_SIZE];
                sscanf(receiveDataBuffer, "0 %s %s", buffer, buffer2);
                if (strcmp(buffer2, "add") == 0)
                {
                    printf("New user added: %s\n", uuid);
                }
                else
                {
                    printf("User updated: %s\n", uuid);
                }
            }
            else if (strstr(receiveDataBuffer, "255") != NULL)
            {
                printf("%s\n", receiveDataBuffer);
            }
        }
    }

    close(socketUserServer);
    close(socketLocationServer);
    exit(EXIT_SUCCESS);
}