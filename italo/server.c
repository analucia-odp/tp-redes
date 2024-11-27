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

void usage(int argc, char **argv)
{
    printf("usage: %s <v4|v6> <server port\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
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
        usage(argc, argv);
    }

    if (server_sockaddr_init(argv[1], argv[2], &storage) != 0)
    {
        usage(argc, argv);
    }

    // socket
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
    // bind
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    bind_response = bind(socket_response, addr, sizeof(storage));
    if (bind_response != 0)
    {
        logexit("bind");
    }

    // listen
    listen_response = listen(socket_response, 10); // quantidade de conexões que podem estar pendentes para tratamento
    if (listen_response != 0)
    {
        logexit("listen");
    }

    addrtostr(addr, addrstr, BUFFER_SIZE);
    printf("bound to %s, waiting connections\n", addrstr);

    fd_set sockets_set, copy;
    int max_count_sockets = 0;
    FD_ZERO(&sockets_set);                 // Limpa o conjunto de sockets de leitura
    FD_SET(socket_response, &sockets_set); // Adiciona o socket do servidor ao conjunto

    //printf("\nSocket response = %d", socket_response);

    max_count_sockets = socket_response + 1;

    char client_addr_str[BUFFER_SIZE];

    while (1)
    {

        copy = sockets_set;
        // Aguarda a atividade em algum dos sockets monitorados
        int select_response = select(max_count_sockets, &copy, NULL, NULL, NULL);

        //printf("\nSelect response = %d", select_response);

        if (select_response < 0)
            logexit("select");

        for (int i = 0; i < max_count_sockets; i++)
        {
            bool fd_set_contain_socket = FD_ISSET(i, &copy);
            if (fd_set_contain_socket)
            {
                //printf("\nSet contain %d socket", i);
                //printf("\nSocket response = %d", socket_response);
                if (i == socket_response)
                {
                    struct sockaddr *client_addr = (struct sockaddr *)(&client_storage);
                    socklen_t client_addr_len = sizeof(client_storage);
                    int new_socket_client = accept(socket_response, client_addr, &client_addr_len);
                    //printf("\nNew socket client = %d", new_socket_client);
                    if (new_socket_client == -1)
                    {
                        logexit("accept");
                    }
                    addrtostr(client_addr, client_addr_str, BUFFER_SIZE);
                    printf("[log] connection from %s\n", client_addr_str);
                    FD_SET(new_socket_client, &sockets_set);
                    if (new_socket_client > max_count_sockets)
                    {
                        max_count_sockets = new_socket_client + 1;
                    }
                }
                else
                {
                    // recebe a mensagem do cliete
                    char buffer_data[BUFFER_SIZE];
                    memset(buffer_data, 0, BUFFER_SIZE);
                    size_t bytes_counter = recv(i, buffer_data, BUFFER_SIZE - 1, 0);

                    if (bytes_counter <= 0)
                    {
                        printf("[log] client disconnected\n");
                        close(i);
                        FD_CLR(i, &sockets_set);
                    }
                    else
                    {

                        printf("[msg] %s, %d bytes: %s\n", client_addr_str, (int)bytes_counter, buffer_data);

                        // manda a resposta para o cliente
                        sprintf(buffer_data, "remote endpoint: %.1000s\n", client_addr_str);
                        size_t bytes_count_send;
                        bytes_count_send = send(i, buffer_data, strlen(buffer_data) + 1, 0);
                        if (bytes_count_send != strlen(buffer_data) + 1)
                            logexit("send");
                    }
                }
            }
        }
    }

    exit(EXIT_SUCCESS);
}