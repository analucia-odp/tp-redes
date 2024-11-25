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
    printf("usage: %s <serverIP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv){
    int socket_response;
    int connect_response;
    char addrstr[BUFFER_SIZE];
    char data_buffer[BUFFER_SIZE];
    int count;
    unsigned total = 0;

    if (argc < 3){
        usage(argc, argv);
    }

    
    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    socket_response = socket(storage.ss_family, SOCK_STREAM, 0);
    if (socket_response == -1){
        logexit("socket");
    }    
    struct sockaddr *addr = (struct sockaddr *)(&storage); //instanciacao do ponteiro e fiz a conversao do tipo do ponteiro
     //o addr é um ponteiro de tamanho indefinido, uma vez que ele aponta para a estrutura do storage que de fato armazena o endereço
    connect_response = connect(socket_response, addr, sizeof(storage));
    if (connect_response != 0){
        logexit("connect");
    }

    addrtostr(addr, addrstr, BUFFER_SIZE);
    printf("connect to %s\n", addrstr);
    memset(data_buffer, 0, BUFFER_SIZE);
    printf("mensagem >");
    fgets(data_buffer, BUFFER_SIZE-1, stdin);
    count = send(socket_response, data_buffer, strlen(data_buffer)+1 , 0);
    
    if (count != strlen(data_buffer)+1){
        logexit("send");
    }

    memset(data_buffer, 0, BUFFER_SIZE);
    while(1){ //vai recebendo os pacotes de pouquinho e pouquinho
        //estratégia para colocar os bytes em ordem no buffer - JANELA DE RECEPÇÃO
        count = recv(socket_response, data_buffer + total, BUFFER_SIZE - total, 0);
        if (count == 0){//Significa que o servidor já terminou de receber tudo e posso finalizar a conexão
            //Connection terminated
            break;
        }
        total += count;
    }
    close(socket_response);

    printf("received %d bytes\n", total);
    puts(data_buffer);
    exit(EXIT_SUCCESS);
}