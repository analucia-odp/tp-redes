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
#include <unistd.h>

#define BUFFER_SIZE 1024

struct user
{
    char *userId;
    short int hasPermission;
    short int locId;
};

struct client
{
    short int clientId;
    short int locId;
};

void init_setsockopt_ipv6(int socket_response)
{
    int enable = 0;

    int setsockopt_result = setsockopt(socket_response, IPPROTO_IPV6, IPV6_V6ONLY, &enable, sizeof(enable));
    if (setsockopt_result == -1)
    {
        logexit("setsockopt IPV6_V6ONLY");
    }
}

void init_setsockopt_reuse(int socket_response)
{
    int enable = 1;

    int setsockopt_result = setsockopt(socket_response, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (setsockopt_result == -1)
    {
        logexit("setsockopt SO_REUSEADDR");
    }
}

void initialize_server_structure(struct sockaddr_storage *storage, const char *port)
{
    int isServerInit = server_sockaddr_init(port, storage);
    if (isServerInit == -1)
    {
        logexit("server_sockaddr_init");
    }
}

int open_socket(struct sockaddr_storage *storage, const char *port)
{
    int socket_response = socket(AF_INET6, SOCK_STREAM, 0);
    if (socket_response == -1)
    {
        logexit("socket");
    }

    init_setsockopt_ipv6(socket_response);
    init_setsockopt_reuse(socket_response);
    initialize_server_structure(storage, port);

    return socket_response;
}

int passive_open(int socket_response, struct sockaddr_storage storage)
{
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    int bind_response = bind(socket_response, addr, sizeof(storage));
    if (bind_response != 0)
    {
        return 0;
    }

    int listen_response = listen(socket_response, 10);
    if (listen_response != 0)
    {
        logexit("listen");
    }

    return 1;
}

int active_open(int socket, struct sockaddr_storage *storage)
{
    struct sockaddr *addr = (struct sockaddr *)(storage);
    int connect_response = connect(socket, addr, sizeof(*storage));
    if (connect_response != 0)
    {
        logexit("connect");
    }
    return connect_response;
}

int accept_socket(int socket_response, struct sockaddr_storage client_storage)
{
    struct sockaddr *clientAddress = (struct sockaddr *)(&client_storage);
    socklen_t clientAddressLen = sizeof(client_storage);
    int acceptConnectionSocketClient = accept(socket_response, clientAddress, &clientAddressLen);
    if (acceptConnectionSocketClient == -1)
    {
        logexit("accept");
    }

    char clientAddress_str[BUFFER_SIZE];
    addrtostr(clientAddress, clientAddress_str, BUFFER_SIZE);
    // printf("Conexão estabelecida com %s\n", clientAddress_str);

    return acceptConnectionSocketClient;
}

int receive_message(int acceptConnectionSocketClient, char *receiveBufferDataClient)
{
    memset(receiveBufferDataClient, 0, BUFFER_SIZE);
    size_t bytes_counter = recv(acceptConnectionSocketClient, receiveBufferDataClient, BUFFER_SIZE - 1, 0);
    if (bytes_counter == 0)
    {
        return 0;
    }
    return 1;
}

void send_message(int acceptConnectionSocketClient, char *sendBufferDataClient)
{
    size_t bytes_count_send;
    bytes_count_send = send(acceptConnectionSocketClient, sendBufferDataClient, strlen(sendBufferDataClient) + 1, 0);
    if (bytes_count_send != strlen(sendBufferDataClient) + 1)
        logexit("send");
}

void convertToString(char *str, int message)
{
    sprintf(str, "%d", message);
}

void log_error_message_invalid_arguments(int argc, char **argv)
{
    printf("Invalid arguments!\n");
    printf("Example: %s 40000 50000\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int bind_response;
    int listen_response;
    int accept_connection_socket;

    char addrstr[BUFFER_SIZE];

    struct sockaddr_storage client_storage;
    struct sockaddr_storage storage;
    struct sockaddr_storage storage_peer;

    // Configurações iniciais
    if (argc < 3)
    {
        log_error_message_invalid_arguments(argc, argv);
    }

    // Socket
    const char *portPeer = argv[1];
    const char *portClient = argv[2];

    int socket_response = open_socket(&storage, portClient);
    int socket_response_peer = open_socket(&storage_peer, portPeer);

    // --------------- Abertura Passiva para clientes ------------
    int passive_open_clients_ok = passive_open(socket_response, storage);

    // --------------- Abertura Passiva para servidores ------------
    char sendBufferDataPeer[BUFFER_SIZE];
    char receiveBufferDataPeer[BUFFER_SIZE];
    int passive_open_server_ok = passive_open(socket_response_peer, storage_peer);
    int serverIds[BUFFER_SIZE];
    int count_server = 0;

    if (passive_open_server_ok)
    {
        int active_connections_peer = 0;
        printf("No peer found, starting to listen...\n");
        int accept_connection_socket_peer = accept_socket(socket_response_peer, storage_peer);
        active_connections_peer++;
        if (active_connections_peer > 1)
        {
            // printf("Peer limit exceeded\n");
            memset(sendBufferDataPeer, 0, BUFFER_SIZE);
            snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %s", ERROR, "Peer limit exceeded");
            send_message(accept_connection_socket_peer, sendBufferDataPeer);
            close(accept_connection_socket_peer);
            active_connections_peer--;
        }
        else
        {
            serverIds[count_server] = getpid();
            count_server++;
            printf("Peer %d added\n", serverIds[count_server]);
            memset(sendBufferDataPeer, 0, BUFFER_SIZE);
            snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %d", RES_CONNPEER, serverIds[count_server]);
            send_message(accept_connection_socket_peer, sendBufferDataPeer);
            int receive_response = receive_message(accept_connection_socket_peer, receiveBufferDataPeer);
            if (strstr(receiveBufferDataPeer, "18") != NULL)
            {
                int serverId;
                sscanf(receiveBufferDataPeer, "18 %d", &serverId);
                printf("New Peer ID: %d\n", serverId);
            }
        }
    }
    else
    {
        // ---------------- Abertura ativa --------------------------------
        int connect_peer_server = active_open(socket_response_peer, &storage_peer);
        int receive_response = receive_message(socket_response_peer, receiveBufferDataPeer);
        if (strstr(receiveBufferDataPeer, "255") != NULL)
        {
            printf("%s\n", receiveBufferDataPeer);
        }
        if (strstr(receiveBufferDataPeer, "18") != NULL)
        {
            int serverId;
            sscanf(receiveBufferDataPeer, "18 %d", &serverId);
            printf("New Peer ID: %d\n", serverId);
            serverIds[count_server] = getpid();
            count_server++;
            printf("Peer %d connected\n", serverIds[count_server]);
            memset(sendBufferDataPeer, 0, BUFFER_SIZE);
            snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %d", RES_CONNPEER, serverIds[count_server]);
            send_message(socket_response_peer, sendBufferDataPeer);
        }
    }

    // addrtostr(addr, addrstr, BUFFER_SIZE);
    char sendBufferDataClient[BUFFER_SIZE];
    char receiveBufferDataClient[BUFFER_SIZE];
    // struct user *users = NULL;
    struct user users[BUFFER_SIZE];
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        users[i].hasPermission = 0;
        users[i].locId = 0;
    }
    struct client clients[BUFFER_SIZE];
    int count_client = 0;
    int count_user = 0;

    int activeConnections = 0;

    while (1)
    {
        // Accept
        int acceptConnectionSocketClient = accept_socket(socket_response, client_storage);
        activeConnections++;

        while (1)
        {
            if (activeConnections > 10)
            {
                printf("Client limit exceeded\n");
                memset(sendBufferDataClient, 0, BUFFER_SIZE);
                snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "Client limit exceeded");
                send_message(acceptConnectionSocketClient, sendBufferDataClient);
                close(acceptConnectionSocketClient);
                activeConnections--;
                break;
            }
            // Recebe mensagem do usuário
            int receive_message_response = receive_message(acceptConnectionSocketClient, receiveBufferDataClient);
            // printf("Mensagem recebida: %s\n", receiveBufferDataClient);
            if (!receive_message_response)
            {
                activeConnections--;
                break;
            }

            char str[20];
            // ------------- REQ_CONN -------------
            sprintf(str, "%d", REQ_CONN);
            if (strstr(receiveBufferDataClient, str) != NULL)
            {
                clients[count_client].clientId = getpid();
                short int locId;
                sscanf(receiveBufferDataClient, "20 %hd", &locId);
                clients[count_client].locId = locId;
                memset(sendBufferDataClient, 0, BUFFER_SIZE);
                snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %d", RES_CONN, clients[count_client].clientId);
                printf("Client %d added (Loc %d)\n", clients[count_client].clientId, clients[count_client].locId);
                count_client++;
            }

            // ------------- REQ_DISC -------------
            sprintf(str, "%d", REQ_DISC);
            if (strstr(receiveBufferDataClient, str) != NULL)
            {
                short int clientId;
                sscanf(receiveBufferDataClient, "22 %hd", &clientId);
                int clientDisconnect = 0;
                int oldLocId = 0;
                for (int i = 0; i < BUFFER_SIZE; i++)
                {
                    if (clients[i].clientId == clientId)
                    {
                        clients[i].clientId = -1;
                        oldLocId = clients[i].locId;
                        clients[i].locId = 0;
                        printf("Client removed %hd (Loc %hd)\n", clientId, oldLocId);
                        snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", OK, "Succesful disconnect");
                        clientDisconnect = 1;
                        break;
                    }
                }

                if (!clientDisconnect)
                {
                    memset(sendBufferDataClient, 0, BUFFER_SIZE);
                    snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "Client not found");
                }
            }

            // ------------- REQ_USRADD -------------
            sprintf(str, "%d", REQ_USRADD);
            if (strstr(receiveBufferDataClient, str) != NULL)
            {
                int hasPermission;
                char userId[10];
                sscanf(receiveBufferDataClient, "33 %s %d", userId, &hasPermission);
                int findUser = 0;
                for (int i = 0; i < count_user; i++)
                {
                    if (strcmp(users[i].userId, userId) == 0)
                    {
                        users[i].hasPermission = hasPermission;
                        memset(sendBufferDataClient, 0, BUFFER_SIZE);
                        snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", OK, "Successful update");
                        findUser = 1;
                        break;
                    }
                }

                if (!findUser)
                {
                    if (count_user >= 30)
                    {
                        memset(sendBufferDataClient, 0, BUFFER_SIZE);
                        snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "User limit exceeded");
                    }
                    else
                    {
                        users[count_user].userId = userId;
                        users[count_user].hasPermission = hasPermission;
                        memset(sendBufferDataClient, 0, BUFFER_SIZE);
                        snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", OK, "Successful add");
                        count_user++;
                    }
                }
            }

            // ------------- REQ_USRADD -------------
            sprintf(str, "%d", REQ_USRLOC);
            if (strstr(receiveBufferDataClient, str) != NULL)
            {
                char userId[10];
                sscanf(receiveBufferDataClient, "38 %s", userId);
                int findUser = 0;
                for (int i = 0; i < count_user; i++)
                {
                    if (strcmp(users[i].userId, userId) == 0)
                    {
                        memset(sendBufferDataClient, 0, BUFFER_SIZE);
                        snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %d", RES_USRLOC, users[i].locId);
                        findUser = 1;
                        break;
                    }
                }

                if (!findUser)
                {
                    memset(sendBufferDataClient, 0, BUFFER_SIZE);
                    snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "User not found");
                }
            }

            // Envia mensagem para o client
            send_message(acceptConnectionSocketClient, sendBufferDataClient);
        }

        close(acceptConnectionSocketClient);
    }

    close(socket_response);
    exit(EXIT_SUCCESS);
}

// -- client nao carrega a lista de usuarios
// vetor de ids de usuários - char **
// while (n conecta)
// conecta()
// se falhar
// listen()
// sleep(1)
// array para clientes
// struct que condensa o locId, idUser e isSpecial
// Inicializar minhas estruturas, loc = 0 significa que nao foi preenchido
// o cliente inicializa com sua localizacao, que é seu predio
// se o usuário entrar naquele predio, com o comando in, significa que ele está na localização daquele client

// OPCODE
//  CHAR* PAYLOAD

// MAIS PRA FRENTE
//  - OPCODE - 1 BYTE
//  QUANTIDADE DE CARTEIRINHAS
// CARTEIRINHA\0CARTEIRINHA2\0CARTEIRINHA3\0