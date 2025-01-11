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

#define MAX_USERS 30
#define MAX_CLIENTS 10

struct user
{
    char userId[11];
    short int hasPermission;
    short int locId;
};

struct client
{
    short int clientId;
    short int locId;
    int socket;
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

int find_user(int count, struct user *users, const char *userId)
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

int find_client_by_socket(struct client *clients, int socket)
{
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        if (clients[i].socket == socket)
        {
            return i;
        }
    }

    return -1;
}
void log_error_message_invalid_arguments(int argc, char **argv)
{
    printf("Invalid arguments!\n");
    printf("Example: %s 40000 50000\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    // socket
    int bind_response;
    int listen_response;
    int accept_connection_socket;
    int connection_socket_peer;

    // Buffers de dados
    char sendBufferDataClient[BUFFER_SIZE];
    char receiveBufferDataClient[BUFFER_SIZE];
    char sendBufferDataPeer[BUFFER_SIZE];
    char receiveBufferDataPeer[BUFFER_SIZE];

    // id conexão
    int server_peer_id;
    int server_active_connection_id;
    int server_passive_connection_id;

    // counts
    int count_server = 0;
    int count_client = 0;
    int count_user = 0;
    int active_connections = 0;
    int active_connections_peer = 0;

    // sockaddr
    struct sockaddr_storage client_storage;
    struct sockaddr_storage storage;
    struct sockaddr_storage storage_peer;

    // descritores
    fd_set master_set, read_set;
    int fdmax;

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
    int passive_open_server_ok = passive_open(socket_response_peer, storage_peer);

    if (passive_open_server_ok)
    {
        printf("No peer found, starting to listen...\n");
    }
    else
    {
        // ---------------- Abertura ativa e Abertura de comunicação --------------------------------
        int connect_peer_server = active_open(socket_response_peer, &storage_peer);
        memset(sendBufferDataPeer, 0, BUFFER_SIZE);
        snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d", REQ_CONNPEER);
        send_message(socket_response_peer, sendBufferDataPeer);
        int receive_response = receive_message(socket_response_peer, receiveBufferDataPeer);
        if (strncmp(receiveBufferDataPeer, "255", strlen("255")) == 0)
        {
            char message[BUFFER_SIZE];
            sscanf(receiveBufferDataPeer, "255 %[^\n]", message);
            printf("%s\n", message);
        }
        if (strncmp(receiveBufferDataPeer, "18", strlen("18")) == 0)
        {
            // Pego meu Identificador
            sscanf(receiveBufferDataPeer, "18 %d", &server_active_connection_id);
            printf("New Peer ID: %d\n", server_active_connection_id);
            // Gero um identificador para meu outro servidor
            server_peer_id = count_server;
            count_server++;
            printf("Peer %d connected\n", server_peer_id);
            memset(sendBufferDataPeer, 0, BUFFER_SIZE);
            snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %d", RES_CONNPEER, server_peer_id);
            send_message(socket_response_peer, sendBufferDataPeer);
        }
    }

    struct user users[MAX_USERS];
    for (int i = 0; i < MAX_USERS; i++)
    {
        strncpy(users[i].userId, "", 11);
        users[i].hasPermission = 0;
        users[i].locId = -1;
    }
    struct client clients[BUFFER_SIZE];

    // Limpa conjunto de descritores
    FD_ZERO(&master_set);
    FD_ZERO(&read_set);

    // Adiciona o socket de resposta ao conjunto
    FD_SET(socket_response, &master_set);
    FD_SET(socket_response_peer, &master_set);
    FD_SET(STDIN_FILENO, &master_set);
    fdmax = (socket_response > socket_response_peer) ? socket_response : socket_response_peer;
    if (STDIN_FILENO > fdmax)
    {
        fdmax = STDIN_FILENO;
    }

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
                    active_connections++;
                    if (active_connections > MAX_CLIENTS)
                    {
                        memset(sendBufferDataClient, 0, BUFFER_SIZE);
                        snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "Client limit exceeded");
                        send_message(acceptConnectionSocketClient, sendBufferDataClient);
                        close(acceptConnectionSocketClient);
                        active_connections--;
                        continue;
                    }
                    FD_SET(acceptConnectionSocketClient, &master_set);
                    if (acceptConnectionSocketClient > fdmax)
                    {
                        fdmax = acceptConnectionSocketClient;
                    }
                }
                // se o descritor for o socket do servidor peer - SU
                else if (i == socket_response_peer && passive_open_server_ok)
                {
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
                        continue;
                    }
                    FD_SET(accept_connection_socket_peer, &master_set);
                    connection_socket_peer = accept_connection_socket_peer;
                    if (accept_connection_socket_peer > fdmax)
                    {
                        fdmax = accept_connection_socket_peer;
                    }
                }
                else if (i == STDIN_FILENO)
                {
                    char input[BUFFER_SIZE];
                    if (fgets(input, sizeof(input), stdin) != NULL)
                    {
                        // Fechamento de Conexão
                        if (strcmp(input, "kill\n") == 0)
                        {
                            int server_id = passive_open_server_ok ? server_passive_connection_id : server_active_connection_id;
                            int socket = passive_open_server_ok ? connection_socket_peer : socket_response_peer;
                            // SEND REQ_DISCPEER
                            memset(sendBufferDataPeer, 0, BUFFER_SIZE);
                            snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %d", REQ_DISCPEER, server_id);
                            send_message(socket, sendBufferDataPeer);

                            // RCV REQ_DISCPEER
                            receive_message(socket, receiveBufferDataPeer);
                            if (strncmp(receiveBufferDataPeer, "255", strlen("255")) == 0)
                            {
                                char message[BUFFER_SIZE];
                                sscanf(receiveBufferDataPeer, "255 %[^\n]", message);
                                printf("%s\n", message);
                            }
                            else if (strncmp(receiveBufferDataPeer, "0", strlen("0")) == 0)
                            {
                                printf("Successful disconnect\n");
                                printf("Peer %d disconnected\n", server_peer_id);
                                server_peer_id = -1;
                                if (passive_open_clients_ok)
                                {
                                    passive_open_clients_ok = 0;
                                }
                                exit(EXIT_SUCCESS);
                            }
                        }
                        else
                        {
                            printf("Invalid arguments\n");
                        }
                    }
                }
                // se não for, é o socket do cliente ou do servidor peer
                else
                {
                    char str[20];

                    if (passive_open_server_ok && i == connection_socket_peer) // SU
                    {
                        int receive_message_response_peer = receive_message(i, receiveBufferDataPeer);
                        // ----------------------------- MENSAGENS DO PEER ---------------------------------------------
                        // ------------- REQ_CONNPEER -------------
                        sprintf(str, "%d", REQ_CONNPEER);
                        if (strncmp(receiveBufferDataPeer, str, strlen(str)) == 0)
                        {

                            server_peer_id = count_server;
                            printf("Peer %d connected\n", server_peer_id);
                            memset(sendBufferDataPeer, 0, BUFFER_SIZE);
                            snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %d", RES_CONNPEER, server_peer_id);
                            count_server++;
                            // Envia mensagem para o peer
                            send_message(i, sendBufferDataPeer);
                        }

                        // ------------- RES_CONNPEER -------------
                        sprintf(str, "%d", RES_CONNPEER);
                        if (strncmp(receiveBufferDataPeer, str, strlen(str)) == 0)
                        {
                            sscanf(receiveBufferDataPeer, "18 %d", &server_passive_connection_id);
                            printf("New Peer ID: %d\n", server_passive_connection_id);
                        }

                        // ------------- REQ_DISCPEER -------------
                        sprintf(str, "%d", REQ_DISCPEER);
                        if (strncmp(receiveBufferDataPeer, str, strlen(str)) == 0)
                        {
                            int peerId;
                            sscanf(receiveBufferDataPeer, "19 %d", &peerId);
                            if (peerId == server_peer_id)
                            {
                                memset(sendBufferDataPeer, 0, BUFFER_SIZE);
                                snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %s", OK, "Successful disconnect");
                                printf("Peer %d disconnected\n", server_peer_id);
                                send_message(i, sendBufferDataPeer);
                                close(i);
                                FD_CLR(i, &master_set);
                                active_connections_peer--;
                                for (int i = 0; i < count_client; i++)
                                {
                                    if (clients[i].clientId != -1)
                                    {
                                        clients[i].clientId = -1;
                                        close(clients[i].socket);
                                        FD_CLR(clients[i].socket, &master_set);
                                        active_connections--;
                                    }
                                }
                                printf("No peer found, starting to listen...\n");
                            }
                            else
                            {
                                memset(sendBufferDataPeer, 0, BUFFER_SIZE);
                                snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %s", ERROR, "Peer not found");
                                send_message(i, sendBufferDataPeer);
                            }
                        }
                        // ------------- REQ_USRAUTH -------------
                        sprintf(str, "%d", REQ_USRAUTH);
                        if (strncmp(receiveBufferDataPeer, str, strlen(str)) == 0)
                        {
                            char userId[11] = {0};
                            sscanf(receiveBufferDataPeer, "42 %10s", userId);
                            printf("REQ_USRAUTH %s\n", userId);
                            int findUserId = find_user(count_user, users, userId);
                            memset(sendBufferDataPeer, 0, BUFFER_SIZE);
                            if (findUserId != -1)
                            {
                                if (users[findUserId].hasPermission)
                                {
                                    snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d 1", RES_USRAUTH);
                                }
                                else
                                {
                                    snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d 0", RES_USRAUTH);
                                }
                            }
                            else
                            {
                                snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d -1", RES_USRAUTH);
                            }

                            send_message(i, sendBufferDataPeer);
                        }
                    }
                    else if (!passive_open_server_ok && i == socket_response_peer)
                    { // SL respondendo o SU
                        int receive_message_response_peer = receive_message(i, receiveBufferDataPeer);
                        // ------------- REQ_LOCREG -------------
                        sprintf(str, "%d", REQ_LOCREG);
                        if (strncmp(receiveBufferDataPeer, str, strlen(str)) == 0)
                        {
                            char userId[11] = {0};
                            int locId;
                            int oldLocId;
                            sscanf(receiveBufferDataPeer, "36 %10s %d", userId, &locId);
                            printf("REQ_LOCREG %s %d\n", userId, locId);
                            int findUserId = find_user(count_user, users, userId);
                            if (findUserId != -1)
                            {
                                oldLocId = users[findUserId].locId;
                                users[findUserId].locId = locId;
                            }
                            else
                            {
                                strcpy(users[count_user].userId, userId);
                                oldLocId = -1;
                                users[count_user].locId = locId;
                                count_user++;
                            }
                            memset(sendBufferDataPeer, 0, BUFFER_SIZE);
                            snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %d", RES_LOCREG, oldLocId);
                        }

                        // ------------- REQ_DISCPEER -------------
                        sprintf(str, "%d", REQ_DISCPEER);
                        if (strncmp(receiveBufferDataPeer, str, strlen(str)) == 0)
                        {
                            int peerId;
                            sscanf(receiveBufferDataPeer, "19 %d", &peerId);
                            if (peerId == server_peer_id)
                            {
                                memset(sendBufferDataPeer, 0, BUFFER_SIZE);
                                snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %s", OK, "Successful disconnect");
                                printf("Peer %d disconnected\n", server_peer_id);
                                send_message(i, sendBufferDataPeer);
                                close(i);
                                FD_CLR(i, &master_set);
                                active_connections_peer--;
                                for (int i = 0; i < count_client; i++)
                                {
                                    if (clients[i].clientId != -1)
                                    {
                                        clients[i].clientId = -1;
                                        close(clients[i].socket);
                                        FD_CLR(clients[i].socket, &master_set);
                                        active_connections--;
                                    }
                                }
                                sleep(1);
                                // --------------- Abertura Passiva para servidores ------------
                                close(socket_response_peer);
                                FD_CLR(socket_response_peer, &master_set);
                                socket_response_peer = open_socket(&storage_peer, portPeer);
                                FD_SET(socket_response_peer, &master_set);
                                passive_open_server_ok = passive_open(socket_response_peer, storage_peer);

                                if (passive_open_server_ok)
                                {
                                    printf("No peer found, starting to listen...\n");
                                }
                                continue;
                            }
                            else
                            {
                                memset(sendBufferDataPeer, 0, BUFFER_SIZE);
                                snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %s", ERROR, "Peer not found");
                                send_message(i, sendBufferDataPeer);
                            }
                        }
                        send_message(i, sendBufferDataPeer);
                    }
                    else
                    {
                        int receive_message_response = receive_message(i, receiveBufferDataClient);
                        // ----------------------------- MENSAGENS DE CLIENTES ---------------------------------------------
                        // ------------- REQ_CONN -------------
                        sprintf(str, "%d", REQ_CONN);
                        if (strncmp(receiveBufferDataClient, str, strlen(str)) == 0)
                        {
                            clients[count_client].clientId = count_client;
                            clients[count_client].socket = i;
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
                                active_connections--;
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
                            char userId[11] = {0};
                            sscanf(receiveBufferDataClient, "33 %10s %d", userId, &hasPermission);
                            printf("REQ_USRADD %s %d\n", userId, hasPermission);
                            int findUserId = find_user(count_user, users, userId);
                            if (findUserId != -1)
                            {
                                users[findUserId].hasPermission = hasPermission;
                                memset(sendBufferDataClient, 0, BUFFER_SIZE);
                                snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", OK, "Successful update");
                                printf("OK 3\n");
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
                                    strcpy(users[count_user].userId, userId);
                                    users[count_user].hasPermission = hasPermission;
                                    count_user++;
                                    memset(sendBufferDataClient, 0, BUFFER_SIZE);
                                    snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", OK, "Successful add");
                                    printf("OK 2\n");
                                }
                            }
                        }

                        // ------------- REQ_USRLOC -------------
                        sprintf(str, "%d", REQ_USRLOC);
                        if (strncmp(receiveBufferDataClient, str, strlen(str)) == 0)
                        {
                            char userId[11] = {0};
                            sscanf(receiveBufferDataClient, "38 %10s", userId);
                            printf("REQ_USRLOC\n");
                            int findUserId = find_user(count_user, users, userId);
                            if (findUserId != -1)
                            {
                                memset(sendBufferDataClient, 0, BUFFER_SIZE);
                                snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %d", RES_USRLOC, users[findUserId].locId);
                                printf("RES_USRLOC %d\n", users[findUserId].locId);
                            }
                            else
                            {
                                memset(sendBufferDataClient, 0, BUFFER_SIZE);
                                snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "User not found");
                                printf("ERROR 18\n");
                            }
                        }

                        // ------------- REQ_USRACCESS -------------
                        sprintf(str, "%d", REQ_USRACCESS);
                        if (strncmp(receiveBufferDataClient, str, strlen(str)) == 0)
                        {
                            char userId[11] = {0};
                            char direction[4] = {0};
                            sscanf(receiveBufferDataClient, "34 %10s %s", userId, direction);
                            printf("REQ_USRACCESS %s %s\n", userId, direction);
                            int findUserId = find_user(count_user, users, userId);
                            if (findUserId != -1)
                            {
                                int loc;
                                if (strcmp(direction, "in") == 0)
                                {
                                    int clientId = find_client_by_socket(clients, i);
                                    if (clientId != -1)
                                    {
                                        loc = clients[clientId].locId;
                                    }
                                }
                                else
                                {
                                    loc = -1;
                                }
                                memset(sendBufferDataPeer, 0, BUFFER_SIZE);
                                snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %s %d", REQ_LOCREG, userId, loc);
                                send_message(connection_socket_peer, sendBufferDataPeer);
                                receive_message(connection_socket_peer, receiveBufferDataPeer);

                                sprintf(str, "%d", RES_LOCREG);
                                if (strncmp(receiveBufferDataPeer, str, strlen(str)) == 0)
                                {
                                    int oldLocId;
                                    sscanf(receiveBufferDataPeer, "37 %d", &oldLocId);
                                    memset(sendBufferDataClient, 0, BUFFER_SIZE);
                                    snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %d", RES_USRACCESS, oldLocId);
                                    printf("RES_LOCREG %d\n", oldLocId);
                                }
                            }
                            else
                            {
                                memset(sendBufferDataClient, 0, BUFFER_SIZE);
                                snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "User not found");
                                printf("ERROR 18\n");
                            }
                        }
                        // ------------- REQ_LOCLIST -------------
                        sprintf(str, "%d", REQ_LOCLIST);
                        if (strncmp(receiveBufferDataClient, str, strlen(str)) == 0)
                        {
                            char userId[11] = {0};
                            int locId;
                            sscanf(receiveBufferDataClient, "40 %10s %d", userId, &locId);
                            printf("REQ_LOCLIST %s %d\n", userId, locId);
                            memset(sendBufferDataPeer, 0, BUFFER_SIZE);
                            snprintf(sendBufferDataPeer, BUFFER_SIZE, "%d %s", REQ_USRAUTH, userId);
                            send_message(socket_response_peer, sendBufferDataPeer);
                            receive_message(socket_response_peer, receiveBufferDataPeer);

                            sprintf(str, "%d", RES_USRAUTH);
                            if (strncmp(receiveBufferDataPeer, str, strlen(str)) == 0)
                            {
                                int permission;
                                sscanf(receiveBufferDataPeer, "43 %d", &permission);
                                printf("RES_USRAUTH %d\n", permission);
                                if (permission == 1)
                                {
                                    char listIdsLocation[MAX_USERS][11];
                                    int countIdsLocation = 0;
                                    for (int i = 0; i < count_user; i++)
                                    {
                                        if (strlen(users[i].userId) > 0 && users[i].locId == locId)
                                        {
                                            strncpy(listIdsLocation[countIdsLocation], users[i].userId, 11);
                                            countIdsLocation++;
                                        }
                                    }

                                    memset(sendBufferDataClient, 0, BUFFER_SIZE);
                                    char idsBuffer[BUFFER_SIZE] = {0}; // Buffer auxiliar para os IDs
                                    int remainingSize = BUFFER_SIZE - 1;
                                    for (int i = 0; i < countIdsLocation; i++)
                                    {
                                        int len = strlen(listIdsLocation[i]);
                                        if (len + 1 > remainingSize)
                                            break;
                                        strncat(idsBuffer, listIdsLocation[i], remainingSize);
                                        remainingSize -= len;
                                        if (i < countIdsLocation - 1)
                                        {
                                            if (1 > remainingSize)
                                            { // Verifica espaço para o espaço adicional
                                                break;
                                            }
                                            strncat(idsBuffer, ", ", remainingSize);
                                            remainingSize -= 1;
                                        }
                                    }
                                    snprintf(sendBufferDataClient, BUFFER_SIZE + 4, "%d %s", RES_LOCLIST, idsBuffer);
                                    printf("RES_LOCLIST %s\n", idsBuffer);
                                }
                                else if (permission == 0)
                                {
                                    memset(sendBufferDataClient, 0, BUFFER_SIZE);
                                    snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "Permission denied");
                                    printf("ERROR 19\n");
                                }
                                else
                                {
                                    memset(sendBufferDataClient, 0, BUFFER_SIZE);
                                    snprintf(sendBufferDataClient, BUFFER_SIZE, "%d %s", ERROR, "User not found");
                                    printf("ERROR 18\n");
                                }
                            }
                        }
                        // Envia mensagem para o client
                        send_message(i, sendBufferDataClient);
                    }
                }
            }
        }
    }

    close(socket_response);
    exit(EXIT_SUCCESS);
}