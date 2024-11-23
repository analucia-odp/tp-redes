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

void usage(int argc, char **argv){
    printf("usage: %s <v4|v6> <server port\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main (int argc, char**argv){
    int socket_response;
    int bind_response; 
    int listen_response;
    int accept_connection_socket;

    char addrstr[BUFFER_SIZE];

    struct sockaddr_storage client_storage; 
    struct sockaddr_storage storage;

    if (argc < 3){
        usage(argc, argv);
    }

    if (server_sockaddr_init(argv[1], argv[2], &storage) != 0){
        usage(argc, argv);
    }

    //socket
    socket_response = socket(storage.ss_family, SOCK_STREAM, 0);
    if (socket_response == -1){
        logexit("socket");
    }
    
    //bind
    struct sockaddr *addr = (struct sockaddr *)(&storage);    
    bind_response = bind(socket_response,addr,sizeof(storage));
    if (bind_response != 0){
        logexit("bind");
    }

    //listen
    listen_response = listen(socket_response, 10); //quantidade de conexões que podem estar pendentes para tratamento
    if (listen_response != 0){
        logexit("listen");
    }

    addrtostr(addr, addrstr, BUFFER_SIZE);
    printf("bound to %s, waiting connections\n", addrstr);

    while(1){
        struct sockaddr *client_addr = (struct sockaddr *)(&client_storage);  
        socklen_t client_addr_len = sizeof(client_storage);
        accept_connection_socket = accept(socket_response, client_addr, &client_addr_len);
        if (accept_connection_socket == -1){
            logexit("accept");
        }

        char client_addr_str[BUFFER_SIZE];
        addrtostr(client_addr, client_addr_str, BUFFER_SIZE);
        printf("[log] connection from %s\n", client_addr_str);

        //recebe a mensagem do cliete
        char buffer_data[BUFFER_SIZE];
        memset(buffer_data, 0, BUFFER_SIZE);
        size_t bytes_counter = recv(accept_connection_socket, buffer_data, BUFFER_SIZE-1, 0);

        printf("[msg] %s, %d bytes: %s\n", client_addr_str, (int)bytes_counter, buffer_data);

        //manda a resposta para o cliente
        sprintf(buffer_data, "remote endpoint: %.1000s\n", client_addr_str);
        size_t bytes_count_send;
        bytes_count_send = send(accept_connection_socket, buffer_data, strlen(buffer_data)+1,0);
        if (bytes_count_send != strlen(buffer_data)+1) logexit("send");
        close(accept_connection_socket);
    }

    exit(EXIT_SUCCESS);
}