#include "commom.h"

void logexit (const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

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

    // descobrir o tipo do endereço (IPV4 ou IPV6)
    // eu nao posso descobrir pelo tamanho?

    struct in_addr inaddr4; // 32-bit IP address
    if (inet_pton(AF_INET, addrstr, &inaddr4))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage; // transformei o ponteiro para apontar para um storage
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; // 128-bit IP address
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) //Texto para representação de rede
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage; // transformei o ponteiro para apontar para um storage
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        // addr6->sin6_addr = inaddr6; //isso é um arranjo de 16 bytes
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize)
{
    int version;
    char addrstr[INET6_ADDRSTRLEN +1] = ""; //REPRESENTAÇAO TEXTUAL DE QUANTO VOCE TEM QUE ARMAZENAR UMA ESTRUTURA IPV4
    uint16_t port;

    if (addr->sa_family == AF_INET){
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr; // transformei o ponteiro para apontar para um storage

        if(!inet_ntop(AF_INET,&(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1)){
            logexit("ntop");
        }

        port = ntohs(addr4->sin_port); // rede para dispositivo
    } 
    else if (addr->sa_family == AF_INET6)
    {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr; // transformei o ponteiro para apontar para um storage

        if(!inet_ntop(AF_INET6,&(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1)){
            logexit("ntop");
        }

        port = ntohs(addr6->sin6_port); // rede para dispositivo
    }
    else
        logexit("unknowm protocol family.");

    if(str) snprintf(str, strsize, "IPV%d %s %hu", version, addrstr, port);
}


int server_sockaddr_init(const char *protocol, const char* port_str, struct sockaddr_storage *storage)
{
    if (protocol == NULL || port_str == NULL)
    {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(port_str);
    if (port == 0)
        return -1;

    port = htons(port); // converte o número da porta do dispositivo para uma porta da rede (host to network short)

    memset(storage, 0, sizeof(*storage)); //Zera o bloco de memória correspondente onde o storage está apontando, evita lixo.

    if (strcmp(protocol, "v4") == 0){ // se o protocolo for IPV4
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage; // Aloco minha struct com o tamanho correspondente IPV4
        addr4->sin_family = AF_INET; //IPV4
        addr4->sin_addr.s_addr = INADDR_ANY; //Aceito qualquer endereço que o computador tenha na interface de rede dele!
        addr4->sin_port = port; //Porta em que o servidor vai se comunicar com os clientes
        return 0;
    }
    else if (strcmp(protocol, "v6") == 0){ // se o protocolo for IPV6
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage; // Aloco minha struct com o tamanho correspondente IPV6
        addr6->sin6_family = AF_INET6; //IPV6
        addr6->sin6_addr = in6addr_any; //Aceito qualquer endereço que o computador tenha na interface de rede dele!
        addr6->sin6_port = port; //Porta em que o servidor vai se comunicar com os clientes
        return 0;
    }
    else{
        return -1; //erro - indicou um protocolo inválido
    }
}