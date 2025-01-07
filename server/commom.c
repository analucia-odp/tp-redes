#include "commom.h"

void logexit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

// --------------- Estruturas de inicialização ------------------

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage)
{
    if (addrstr == NULL || portstr == NULL)
    {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr); // unsigned_short //toda porta tem 16 bits

    if (port == 0)
        return -1;
    port = htons(port); // converte o número da porta dispositivo - rede (host to network short)

    struct in_addr inaddr4; // 32-bit IP address
    if (inet_pton(AF_INET, addrstr, &inaddr4))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage; // transformei o ponteiro para apontar para um storage
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6;                    // 128-bit IP address
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) // Texto para representação de rede
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage; // transformei o ponteiro para apontar para um storage
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize)
{
    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = ""; // REPRESENTAÇAO TEXTUAL DE QUANTO VOCE TEM QUE ARMAZENAR UMA ESTRUTURA IPV4
    uint16_t port;

    if (addr->sa_family == AF_INET)
    {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr; // transformei o ponteiro para apontar para um storage

        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            logexit("ntop");
        }

        port = ntohs(addr4->sin_port); // rede para dispositivo
    }
    else if (addr->sa_family == AF_INET6)
    {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr; // transformei o ponteiro para apontar para um storage

        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            logexit("ntop");
        }

        port = ntohs(addr6->sin6_port); // rede para dispositivo
    }
    else
        logexit("unknowm protocol family.");

    if (str)
        snprintf(str, strsize, "IPV%d %s %hu", version, addrstr, port);
}

int server_sockaddr_init(const char *port_str, struct sockaddr_storage *storage)
{
    if (port_str == NULL)
    {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(port_str);
    if (port == 0)
        return -1;

    port = htons(port); // converte o número da porta do dispositivo para uma porta da rede (host to network short)

    memset(storage, 0, sizeof(*storage)); // Zera o bloco de memória correspondente onde o storage está apontando, evita lixo.
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage; // Aloco minha struct com o tamanho correspondente IPV6
    addr6->sin6_family = AF_INET6;                               // IPV6
    addr6->sin6_addr = in6addr_any;                              // Aceito qualquer endereço que o computador tenha na interface de rede dele!
    addr6->sin6_port = port;                                     // Porta em que o servidor vai se comunicar com os clientes
    return 0;
}

// --------------- Estruturas de manipulação ------------------

void send_message(int socket, char *message){
    size_t bytes_count_send;
    bytes_count_send = send(socket, message, strlen(message) + 1, 0);
    if (bytes_count_send != strlen(message) + 1)
        logexit("send");
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

int receive_message(int socket, char *buffer)
{
    memset(buffer, 0, BUFFER_SIZE);
    size_t bytes_counter = recv(socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_counter == 0)
    {
        return 0;
    }
    return 1;
}

int passive_open(int socket, struct sockaddr_storage storage)
{
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    int bind_response = bind(socket, addr, sizeof(storage));
    if (bind_response != 0)
    {
        return 0;
    }

    int listen_response = listen(socket, 10);
    if (listen_response != 0)
    {
        logexit("listen");
    }

    return 1;
}

int accept_socket(int socket, struct sockaddr_storage storage)
{
    struct sockaddr *clientAddress = (struct sockaddr *)(&storage);
    socklen_t clientAddressLen = sizeof(storage);
    int acceptConnectionSocketClient = accept(socket, clientAddress, &clientAddressLen);
    if (acceptConnectionSocketClient == -1)
    {
        logexit("accept");
    }

    char clientAddress_str[BUFFER_SIZE];
    addrtostr(clientAddress, clientAddress_str, BUFFER_SIZE);

    return acceptConnectionSocketClient;
}