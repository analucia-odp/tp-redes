#include "commom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/select.h>

void log_error_message_invalid_arguments()
{
    printf("Invalid arguments!\n");
    printf("Example IPV4: ./client 127.0.0.1 50000 60000 1\n");
    printf("Example IPV6: ./client ::1 50000 60000 1\n");
    exit(EXIT_FAILURE);
}

int init_socket(const char *addr, const char *port, struct sockaddr_storage *storage)
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

void receiveMessage(int socket, char *buffer)
{
    memset(buffer, 0, BUFFER_SIZE);
    int count = recv(socket, buffer, BUFFER_SIZE, 0);
    if (count <= 0)
    {
        // Conexão fechada pelo servidor
        close(socket);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{

    if (atoi(argv[4]) < 1 || atoi(argv[4]) > 10)
    {
        printf("Invalid argument\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage storageUserServer;
    struct sockaddr_storage storageLocationServer;

    char *addr = argv[1];
    char *portUserServer = argv[2];
    char *portLocationServer = argv[3];
    char *localizationCode = argv[4];

    // Inicialização Sockets
    int socketUserServer = init_socket(addr, portUserServer, &storageUserServer);
    int socketLocationServer = init_socket(addr, portLocationServer, &storageLocationServer);

    // Abertura Ativa
    int connectUserServer = active_open(socketUserServer, &storageUserServer);
    int connectLocationServer = active_open(socketLocationServer, &storageLocationServer);

    char sendDataBuffer[BUFFER_SIZE];
    char receiveDataBuffer[BUFFER_SIZE];

    // Abertura de comunicação de Cliente com Servidores
    memset(sendDataBuffer, 0, BUFFER_SIZE);
    snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%d %s", REQ_CONN, localizationCode);

    // Envia pedido de comunicação
    send_message(socketUserServer, sendDataBuffer);
    send_message(socketLocationServer, sendDataBuffer);

    short int clientId;

    char str[20];
    sprintf(str, "%d", RES_CONN);
    short int clientUserId;
    short int locationUserId;
    int hasConectedUserServer = 0, hasConectedLocationServer = 0;

    // Recebe resposta do servidor de usuario
    receiveMessage(socketUserServer, receiveDataBuffer);

    if (strncmp(receiveDataBuffer, str, strlen(str)) == 0)
    {
        sscanf(receiveDataBuffer, "21 %hd", &clientId);
        printf("SU New ID: %hd\n", clientId);
        clientUserId = clientId;
        hasConectedUserServer = 1;
    }
    if (strncmp(receiveDataBuffer, "255", strlen("255")) == 0)
    {
        char message[BUFFER_SIZE];
        sscanf(receiveDataBuffer, "255 %[^\n]", message);
        printf("%s\n", message);
    }
    // Recebe resposta do servidor de localização
    receiveMessage(socketLocationServer, receiveDataBuffer);
    if (strncmp(receiveDataBuffer, str, strlen(str)) == 0)
    {
        sscanf(receiveDataBuffer, "21 %hd", &clientId);
        printf("SL New ID: %hd\n", clientId);
        locationUserId = clientId;
        hasConectedUserServer = 1;
    }
    if (strncmp(receiveDataBuffer, "255", strlen("255")) == 0)
    {
        char message[BUFFER_SIZE];
        sscanf(receiveDataBuffer, "255 %[^\n]", message);
        printf("%s\n", message);
    }

    if (!hasConectedUserServer && !hasConectedLocationServer)
    {
        exit(EXIT_FAILURE);
    }

    // descritores
    fd_set master_set;
    int fdmax;

    // Limpa conjunto de descritores
    FD_ZERO(&master_set);

    // Adiciona o socket de resposta ao conjunto
    FD_SET(socketUserServer, &master_set);
    FD_SET(socketLocationServer, &master_set);
    FD_SET(STDIN_FILENO, &master_set);
    fdmax = (socketUserServer > socketLocationServer) ? socketUserServer : socketLocationServer;

    if (STDIN_FILENO > fdmax)
    {
        fdmax = STDIN_FILENO;
    }

    while (1)
    {
        // Limpa conjunto de descritores
        FD_ZERO(&master_set);

        // Adiciona o socket de resposta ao conjunto
        FD_SET(socketUserServer, &master_set);
        FD_SET(socketLocationServer, &master_set);
        FD_SET(STDIN_FILENO, &master_set);
        // Espera por atividade em algum dos descritores
        int activity = select(fdmax + 1, &master_set, NULL, NULL, NULL);
        if (activity < 0)
        {
            logexit("select error\n");
        }
        if (FD_ISSET(socketUserServer, &master_set))
        {
            exit(EXIT_SUCCESS);
        }
        if (FD_ISSET(socketLocationServer, &master_set))
        {
            exit(EXIT_SUCCESS);
        }
        if (FD_ISSET(STDIN_FILENO, &master_set))
        {
            // Envio de mensagem
            memset(sendDataBuffer, 0, BUFFER_SIZE);
            fgets(sendDataBuffer, BUFFER_SIZE - 1, stdin);

            // Fechamento de conexão
            if (strcmp(sendDataBuffer, "kill\n") == 0)
            {
                // SEND REQ_DISC
                memset(sendDataBuffer, 0, BUFFER_SIZE);
                snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%d %hd", REQ_DISC, clientUserId);
                send_message(socketUserServer, sendDataBuffer);
                memset(sendDataBuffer, 0, BUFFER_SIZE);
                snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%d %hd", REQ_DISC, locationUserId);
                send_message(socketLocationServer, sendDataBuffer);

                // RCV REQ_DISC
                receiveMessage(socketUserServer, receiveDataBuffer);
                if (strstr(receiveDataBuffer, "0") != NULL)
                {
                    printf("SU Successful disconnect\n");
                }
                else if (strstr(receiveDataBuffer, "255") != NULL)
                {
                    char message[BUFFER_SIZE];
                    sscanf(receiveDataBuffer, "255 %[^\n]", message);
                    printf("%s\n", message);
                }
                receiveMessage(socketLocationServer, receiveDataBuffer);
                if (strstr(receiveDataBuffer, "0") != NULL)
                {
                    printf("SL Successful disconnect\n");
                }
                else if (strstr(receiveDataBuffer, "255") != NULL)
                {
                    char message[BUFFER_SIZE];
                    sscanf(receiveDataBuffer, "255 %[^\n]", message);
                    printf("%s\n", message);
                }
                break;
            }

            // Adiciona usuário
            else if (strstr(sendDataBuffer, "add") != NULL)
            {
                char uuid[11] = {0};
                int isEspecial;
                sscanf(sendDataBuffer, "add %10s %d", uuid, &isEspecial);
                bool hasValidAdd = strlen(uuid) == 10 && (isEspecial == 0 || isEspecial == 1);
                if (hasValidAdd)
                {
                    memset(sendDataBuffer, 0, BUFFER_SIZE);
                    snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%d %s %d", REQ_USRADD, uuid, isEspecial);
                    send_message(socketUserServer, sendDataBuffer);
                    receiveMessage(socketUserServer, receiveDataBuffer);

                    char str[20];
                    sprintf(str, "%d", OK);
                    if (strncmp(receiveDataBuffer, str, strlen(str)) == 0)
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
                        char message[BUFFER_SIZE];
                        sscanf(receiveDataBuffer, "255 %[^\n]", message);
                        printf("%s\n", message);
                    }
                }
                else
                {
                    printf("Invalid arguments\n");
                }
            }

            // Encontra localização usuário
            else if (strstr(sendDataBuffer, "find") != NULL)
            {
                char buffer[BUFFER_SIZE];
                char uuid[11];
                sscanf(sendDataBuffer, "find %10s", uuid);
                bool hasValidArgument = strlen(uuid) == 10;
                if (hasValidArgument)
                {
                    memset(sendDataBuffer, 0, BUFFER_SIZE);
                    snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%d %s", REQ_USRLOC, uuid);
                    send_message(socketLocationServer, sendDataBuffer);
                    receiveMessage(socketLocationServer, receiveDataBuffer);

                    char str[20];
                    sprintf(str, "%d", RES_USRLOC);
                    if (strncmp(receiveDataBuffer, str, strlen(str)) == 0)
                    {
                        short int locId;
                        sscanf(receiveDataBuffer, "39 %hd", &locId);
                        printf("Current location: %d\n", locId);
                    }
                    else if (strstr(receiveDataBuffer, "255") != NULL)
                    {
                        char message[BUFFER_SIZE];
                        sscanf(receiveDataBuffer, "255 %[^\n]", message);
                        printf("%s\n", message);
                    }
                }
                else
                {
                    printf("Invalid arguments\n");
                }
            }
            // Solicita lista de pessoas presentes em uma determinada localização
            else if (strncmp(sendDataBuffer, "inspect", strlen("inspect")) == 0)
            {
                char uuid[11] = {0};
                int locId;
                sscanf(sendDataBuffer, "inspect %10s %d", uuid, &locId);
                bool hasValidArgument = (strlen(uuid) == 10) && (locId == -1 || (locId >= 1 && locId <= 10));
                if (hasValidArgument)
                {
                    memset(sendDataBuffer, 0, BUFFER_SIZE);
                    snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%d %s %d", REQ_LOCLIST, uuid, locId);
                    send_message(socketLocationServer, sendDataBuffer); // Envia apenas para o servidor de ulocalização
                    receiveMessage(socketLocationServer, receiveDataBuffer);

                    char str[20];
                    sprintf(str, "%d", RES_LOCLIST);
                    if (strncmp(receiveDataBuffer, str, strlen(str)) == 0)
                    {
                        char idsBuffer[BUFFER_SIZE] = {0};
                        sscanf(receiveDataBuffer, "41 %[^\n]", idsBuffer);
                        printf("List of people at the specified location: %s\n", idsBuffer);
                    }
                    else if (strstr(receiveDataBuffer, "255") != NULL)
                    {
                        char message[BUFFER_SIZE];
                        sscanf(receiveDataBuffer, "255 %[^\n]", message);
                        printf("%s\n", message);
                    }
                }
                else
                {
                    printf("Invalid arguments\n");
                }
            }
            // Solicita entrada e saída de pessoa
            else if (strstr(sendDataBuffer, "in") != NULL || strstr(sendDataBuffer, "out") != NULL)
            {
                char uuid[11] = {0};
                char direction[4] = {0};
                sscanf(sendDataBuffer, "%s %10s", direction, uuid);
                bool hasValidArgument = (strlen(uuid) == 10);
                if (hasValidArgument)
                {
                    memset(sendDataBuffer, 0, BUFFER_SIZE);
                    snprintf(sendDataBuffer, sizeof(sendDataBuffer), "%d %s %s", REQ_USRACCESS, uuid, direction);
                    send_message(socketUserServer, sendDataBuffer); // Envia apenas para o servidor de usuário
                    receiveMessage(socketUserServer, receiveDataBuffer);

                    char str[20];
                    sprintf(str, "%d", RES_USRACCESS);
                    if (strncmp(receiveDataBuffer, str, strlen(str)) == 0)
                    {
                        short int locId;
                        sscanf(receiveDataBuffer, "35 %hd", &locId);
                        printf("Ok. Last location: %d\n", locId);
                    }
                    else if (strstr(receiveDataBuffer, "255") != NULL)
                    {
                        char message[BUFFER_SIZE];
                        sscanf(receiveDataBuffer, "255 %[^\n]", message);
                        printf("%s\n", message);
                    }
                }
                else
                {
                    printf("Invalid arguments\n");
                }
            }
            else
            {
                printf("Invalid command\n");
            }
        }
    }

    close(socketUserServer);
    close(socketLocationServer);
    exit(EXIT_SUCCESS);
}