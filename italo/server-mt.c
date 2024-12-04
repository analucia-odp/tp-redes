// #include "commom.h"

// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <pthread.h>
// #include <sys/select.h>

// // man socket
// #include <sys/types.h>
// #include <sys/socket.h>

// #define BUFFER_SIZE 1024

// struct client_data {
//     int accept_connection_socket;
//     struct sockaddr_storage storage;
// };

// void * client_thread(void *data)
// {
//     struct client_data *client_data = (struct client_data *)data;
//     struct sockaddr *client_addr = (struct sockaddr *)(&client_data->storage);  

//     char client_addr_str[BUFFER_SIZE];
//     addrtostr(client_addr, client_addr_str, BUFFER_SIZE);
//     printf("[log] connection from %s\n", client_addr_str); //Loga dados de conexão com o cliente (Tipo IP, endereço de IP e porta)

//     // -------------------------------- Recebe dados --------------------------------
//     //recebe a mensagem do cliete
//     char buffer_data[BUFFER_SIZE];
//     memset(buffer_data, 0, BUFFER_SIZE); //inicializa o buffer com 0

//     size_t bytes_counter = recv(client_data->accept_connection_socket, buffer_data, BUFFER_SIZE-1, 0); //recebe dados do cliente

//     printf("[msg] %s, %d bytes: %s\n", client_addr_str, (int)bytes_counter, buffer_data);

//     // -------------------------------- Manda a resposta --------------------------------
//     //manda a resposta para o cliente
//     sprintf(buffer_data, "remote endpoint: %.1000s\n", client_addr_str);
//     size_t bytes_count_send;
//     bytes_count_send = send(client_data->accept_connection_socket, buffer_data, strlen(buffer_data)+1,0);

//     if (bytes_count_send != strlen(buffer_data)+1) logexit("send");

//     close(client_data->accept_connection_socket); //fecha conexão

//     pthread_exit(EXIT_SUCCESS);
// }

// void usage(int argc, char **argv){
//     printf("usage: %s <v4|v6> <server port\n", argv[0]);
//     printf("example: %s v4 51511\n", argv[0]);
//     exit(EXIT_FAILURE);
// }

// int main (int argc, char**argv){
//     int bind_response; 
//     int listen_response;
//     int accept_connection_socket;

//     char addrstr[BUFFER_SIZE];

//     // ---------- Inicialização da Estrutura de dados de conexão --------------
//     struct sockaddr_storage client_storage; 
//     struct sockaddr_storage storage; //struct que a gente utiliza para armazenar informações de conexão
//     //sin_family - IPV4 ou IPV6
//     //sin_port - Porta da rede onde o servidor vai se comunicar com os cliente
//     //sin_addr - Endereço Ip que o servidor vai se comunicar
//     //Utiliza-se esse storage para facilitar a alocação de dados, eu não preciso me preocupar em saber a quantidade de bytes q eu tenho
//     //que alocar para IPV4 ou IPV6, ele faz essa alocação pra mim já

//     if (argc < 3){
//         usage(argc, argv);
//     }

//     //arg1 - IPV4 ou IPV6
//     //arg2 - Porta

//     int sockaddr_init = server_sockaddr_init(argv[1], argv[2], &storage); //inicializa minha estrutura com todos os dados necessários

//     if (sockaddr_init == -1){ // Deu erro - Logo a mensagem
//         usage(argc, argv);
//     }

//     //socket - É uma porta entre o processo do servidor e o SO. Faz a interface entre meu programa e meu SO
//     int socket_response = socket(storage.ss_family, SOCK_STREAM, 0); //Tenho que mandar meu protocolo aqui, no último parâmetro da função!
//     if (socket_response == -1){
//         logexit("socket"); //Erro ao processar o socket, ou seja, na inicialização
//     }

//     int enable = 1;
//     int setsockopt_result = setsockopt(socket_response, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)); //Configuração para que eu nao precise esperar 2 min pra iniciar uma nova conexao numa mesma porta
//     // SOL_SOCKET - Configuração está nível socket e não do protocolo específico
//     // SO_REUSEADDR - Reutiliza um endereço local, não trava
//     if (setsockopt_result == -1){
//         logexit("setsockopt");
//     }

//     // ---------------------------------- Abertura passiva -----------------------------------------
//     //bind - Fala pro SO operacional a rede e a porta que o servidor vai esperar conexão 
//     struct sockaddr *addr = (struct sockaddr *)(&storage);    
//     bind_response = bind(socket_response,addr,sizeof(storage));
//     if (bind_response != 0){
//         logexit("bind");
//     }

//     //listen - Fala pro SO que o servidor está pronto para receber conexões
//     listen_response = listen(socket_response, 10); //quantidade de conexões que podem estar pendentes para tratamento
//     if (listen_response != 0){
//         logexit("listen");
//     }

//     addrtostr(addr, addrstr, BUFFER_SIZE); //manipulação para que eu consiga printar as informações
//     printf("bound to %s, waiting connections\n", addrstr);

//     // ---------------------------------- Abertura completa -----------------------------------------
//     while(1){
//         struct sockaddr *client_addr = (struct sockaddr *)(&client_storage); //Cria um novo sockaddr para o cliente
//         socklen_t client_addr_len = sizeof(client_storage);
//         accept_connection_socket = accept(socket_response, client_addr, &client_addr_len); //Aceita
//         if (accept_connection_socket == -1){
//             logexit("accept");
//         }
//         struct client_data *client_data = malloc(sizeof(*client_data)); //aloco um buffer para armazenar os dados do cliente
//         // storage + conexao com o cliente
//         if (!client_data) logexit("malloc");

//         client_data->accept_connection_socket = accept_connection_socket;
//         memcpy(&(client_data->storage), &client_storage, sizeof(client_storage));

//         pthread_t tid;
//         pthread_create(&tid, NULL, client_thread, client_data); //mando para minha thread
//     }

//     exit(EXIT_SUCCESS);
// }