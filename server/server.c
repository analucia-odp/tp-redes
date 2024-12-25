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
#define MAX_USERS 30
#define MAX_CLIENTS 10

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

int find_user(int count, struct user *users, char *userId)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(users[i].userId, userId) == 0)
        {
            return i;
        }
    }
    return -1;
}

int find_client(struct client *clients, short int clientId)
{
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        if (clients[i].clientId == clientId)
        {
            return i;
        }
    }

    return -1;
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
        printf("No peer found, starting to listen...\n");
        int active_connections_peer = 0;
        int accept_connection_socket_peer = accept_socket(socket_response_peer, storage_peer);
        active_connections_peer++;
        // Verifica a quantidade máxima de conexões peer-2-peer = 1 conexão apenas
        if (active_connections_peer > 1)
        {
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
            printf("Peer %d connected\n", serverIds[count_server]);
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

    // Buffers de dados
    char sendBufferDataClient[BUFFER_SIZE];
    char receiveBufferDataClient[BUFFER_SIZE];

    struct user users[BUFFER_SIZE];
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        users[i].hasPermission = 0;
        users[i].locId = 0;
    }
    struct client clients[BUFFER_SIZE];

    // counts
    int count_client = 0;
    int count_user = 0;
    int activeConnections = 0;

    fd_set master_set, read_set;
    int fdmax;

    // Limpa conjunto de descritores
    FD_ZERO(&master_set);
    FD_ZERO(&read_set);

    // Adiciona o socket de resposta ao conjunto
    FD_SET(socket_response, &master_set);
    fdmax = socket_response;

    while (1)
    {
        // Copia o conjunto de descritores
        read_set = master_set;

        // Espera por atividade em algum dos descritores
        int activity = select(fdmax + 1, &read_set, NULL, NULL, NULL);
        if (activity < 0)
        {
            logexit("select error\n");
        }

        for (int i = 0; i <= fdmax; i++)
        {
            // monitora atividade em todos os descritores
            if (FD_ISSET(i, &read_set))
            {
                // se o descritor que tiver atividade for o socket do servidor
                if (i == socket_response)
                {
                    int acceptConnectionSocketClient = accept_socket(socket_response, client_storage);
                    activeConnections++;
                    if (activeConnections > MAX_CLIENTS)
                    {
                        printf("Client limit exceeded\n");
                        memset(sendBufferDataClient, 0, BUFFER_SIZE);
                        snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "Client limit exceeded");
                        send_message(acceptConnectionSocketClient, sendBufferDataClient);
                        close(acceptConnectionSocketClient);
                        activeConnections--;
                        continue;
                    }
                    FD_SET(acceptConnectionSocketClient, &master_set);
                    if (acceptConnectionSocketClient > fdmax)
                    {
                        fdmax = acceptConnectionSocketClient;
                    }
                }
                // se não for, é o socket do cliente
                else
                {
                    // Recebe mensagem do usuário
                    int receive_message_response = receive_message(i, receiveBufferDataClient);
                    char str[20];
                    // ------------- REQ_CONN -------------
                    sprintf(str, "%d", REQ_CONN);
                    if (strncmp(receiveBufferDataClient, str, strlen(str)) == 0)
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
                    if (strncmp(receiveBufferDataClient, str, strlen(str)) == 0)
                    {
                        short int clientId;
                        sscanf(receiveBufferDataClient, "22 %hd", &clientId);
                        int findClientId = find_client(clients, clientId);
                        int oldLocId = 0;
                        if (findClientId != -1)
                        {
                            clients[findClientId].clientId = -1;
                            oldLocId = clients[findClientId].locId;
                            clients[findClientId].locId = 0;
                            printf("Client removed %hd (Loc %hd)\n", clientId, oldLocId);
                            snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", OK, "Succesful disconnect");
                            send_message(i, sendBufferDataClient);
                            close(i);
                            FD_CLR(i, &master_set);
                            activeConnections--;
                            continue;
                        }
                        else
                        {
                            memset(sendBufferDataClient, 0, BUFFER_SIZE);
                            snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "Client not found");
                        }
                    }

                    // ------------- REQ_USRADD -------------
                    sprintf(str, "%d", REQ_USRADD);
                    if (strncmp(receiveBufferDataClient, str, strlen(str)) == 0)
                    {
                        int hasPermission;
                        char userId[BUFFER_SIZE];
                        sscanf(receiveBufferDataClient, "33 %s %d", userId, &hasPermission);
                        printf("REQ_USRADD %s %d\n", userId, hasPermission);
                        int findUserId = find_user(count_user, users, userId);
                        if (findUserId != -1)
                        {
                            users[findUserId].hasPermission = hasPermission;
                            memset(sendBufferDataClient, 0, BUFFER_SIZE);
                            snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", OK, "Successful update");
                        }
                        else
                        {
                            if (count_user >= MAX_USERS)
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
                    if (strncmp(receiveBufferDataClient, str, strlen(str)) == 0)
                    {
                        char userId[BUFFER_SIZE];
                        sscanf(receiveBufferDataClient, "38 %s", userId);
                        printf("REQ_USRLOC %s\n", userId);
                        int findUserId = find_user(count_user, users, userId);
                        if (findUserId != -1)
                        {
                            memset(sendBufferDataClient, 0, BUFFER_SIZE);
                            snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %d", RES_USRLOC, users[findUserId].locId);
                        }
                        else
                        {
                            memset(sendBufferDataClient, 0, BUFFER_SIZE);
                            snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "User not found");
                        }
                    }

                    // Envia mensagem para o client
                    send_message(i, sendBufferDataClient);
                }
            }
        }
    }

    close(socket_response);
    exit(EXIT_SUCCESS);
}