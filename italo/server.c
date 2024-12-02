#include "commom.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>
// man socket
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

struct user
{
    unsigned int userId;
    bool hasPermission;
    short int lastLocalization;
};

void log_error_message_invalid_arguments(int argc, char **argv)
{
    printf("Invalid arguments!\n");
    printf("Example: %s 40000 50000\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int socket_response;
    int bind_response;
    int listen_response;
    int accept_connection_socket;

    char addrstr[BUFFER_SIZE];

    struct sockaddr_storage client_storage;
    struct sockaddr_storage storage;

    if (argc < 3)
    {
        log_error_message_invalid_arguments(argc, argv);
    }

    int isServerInit = server_sockaddr_init(argv[1], argv[2], &storage);

    if (isServerInit == -1)
    {
        log_error_message_invalid_arguments(argc, argv);
    }

    // Socket
    socket_response = socket(storage.ss_family, SOCK_STREAM, 0);
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
    bind_response = bind(socket_response, addr, sizeof(storage));
    if (bind_response != 0)
    {
        logexit("bind");
    }

    // Listen
    listen_response = listen(socket_response, 10); // quantidade de conexões que podem estar pendentes para tratamento
    if (listen_response != 0)
    {
        logexit("listen");
    }

    addrtostr(addr, addrstr, BUFFER_SIZE);
    printf("Servidor executando em %s\n", addrstr);
    printf("Esperando por novas conexões...\n");

    char sendBufferDataClient[BUFFER_SIZE];
    char receiveBufferDataClient[BUFFER_SIZE];
    // struct user *users = NULL;
    // size_t user_count = 0;

    int activeConnections = 0;

    while (1)
    {
        // if (activeConnections >= 10){
        //     printf("Client limit exceeded");
        //     memset(sendBufferDataClient, 0, BUFFER_SIZE);
        //     strcpy(sendBufferDataClient, "Client limit exceeded");
        //     int bytes_count_send = send(acceptConnectionSocketClient, sendBufferDataClient, strlen(sendBufferDataClient)+1,0);

        // }
        // Accept
        struct sockaddr *clientAddress = (struct sockaddr *)(&client_storage);
        socklen_t clientAddressLen = sizeof(client_storage);
        printf("esperando o accept ..\n");
        int acceptConnectionSocketClient = accept(socket_response, clientAddress, &clientAddressLen);
        printf("accept deu certo..\n");
        if (acceptConnectionSocketClient == -1)
        {
            logexit("accept");
        }
        activeConnections++;

        char clientAddress_str[BUFFER_SIZE];
        addrtostr(clientAddress, clientAddress_str, BUFFER_SIZE);
        printf("Conexão estabelecida com %s\n", clientAddress_str);

        // Cadastro de usuário
        // struct user *temp = realloc(users, (user_count + 1) * sizeof(struct user));
        // if (temp == NULL)
        // {
        //     perror("Failed to allocate memory");
        //     free(users);
        //     exit(EXIT_FAILURE);
        // }
        // users = temp;
        // users[user_count].userId = user_count + 1; // Exemplo de ID de usuário
        // users[user_count].hasPermission = true;    // Exemplo de permissão
        // user_count++;

        // memset(receiveBufferDataClient, 0, BUFFER_SIZE);
        // size_t bytes_counter = recv(acceptConnectionSocketClient, receiveBufferDataClient, BUFFER_SIZE - 1, 0);
        // users[user_count].lastLocalization = atoi(receiveBufferDataClient);
        // printf("Client %d added (Loc %d)", users[user_count].userId, users[user_count].lastLocalization);

        // memset(sendBufferDataClient, 0, BUFFER_SIZE);
        // snprintf(sendBufferDataClient, sizeof(sendBufferDataClient), "SU New ID: %d", users[user_count].userId);
        // int bytes_count_send = send(acceptConnectionSocketClient, sendBufferDataClient, strlen(sendBufferDataClient) + 1, 0);
        while (1)
        {
            // Recebe mensagem do cliente
            memset(receiveBufferDataClient, 0, BUFFER_SIZE);
            size_t bytes_counter = recv(acceptConnectionSocketClient, receiveBufferDataClient, BUFFER_SIZE - 1, 0);
            if (bytes_counter == 0)
            {
                // Conexão fechada pelo cliente
                printf("Client disconnected\n");
                activeConnections--;
                break;
            }

            printf("Mensagem recebida: %s\n", receiveBufferDataClient);

            // Envia mensagem para o client
            memset(sendBufferDataClient, 0, BUFFER_SIZE);
            sprintf(sendBufferDataClient, "Recebi sua mensagem!");
            size_t bytes_count_send;
            bytes_count_send = send(acceptConnectionSocketClient, sendBufferDataClient, strlen(sendBufferDataClient) + 1, 0);
            if (bytes_count_send != strlen(sendBufferDataClient) + 1)
                logexit("send");
        }

        close(acceptConnectionSocketClient);
    }

    close(socket_response);
    exit(EXIT_SUCCESS);
}