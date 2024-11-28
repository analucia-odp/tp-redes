#include "commom.h"

void logexit (const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int buildStorage(const char *internetAddress, const char *portstr, struct sockaddr_storage *storage)
{
    if (internetAddress == NULL || portstr == NULL)
    {
        return -1;
    }

    //Toda porta tem 16 bits
    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0)
        return -1;
    // Converte a porta da rede em uma porta do dispositivo
    port = htons(port); 

    struct in_addr inaddr4; // 32-bit IP address
    int isIPV4 = inet_pton(AF_INET, internetAddress, &inaddr4);
    if (isIPV4)
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage; // transformei o ponteiro para apontar para um storage
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; // 128-bit IP address
    int isIPV6 = inet_pton(AF_INET6, internetAddress, &inaddr6);
    if (isIPV6)
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage; // transformei o ponteiro para apontar para um storage
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void parseAddressToString(const struct sockaddr *addr, char *str, size_t strsize)
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

    if(str) {
        printf("Protocolo: IPV%d", version);
        printf("\nEndereço: %s", addrstr);
        // snprintf("\nPorta: %hu", port);
        // snprintf(str, strsize, "IPV%d %s %hu", version, addrstr, port);
    }
}


int buildStorageServer(const char *portServer, const char* portClient, struct sockaddr_storage *storage)
{
    if (portServer == NULL || portClient == NULL)
    {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portClient);

    if (port == 0)
        return -1;

    port = htons(port); // converte o número da porta do dispositivo para uma porta da rede (host to network short)
    memset(storage, 0, sizeof(*storage)); //Zera o bloco de memória correspondente onde o storage está apontando, evita lixo.

    if (strcmp(portServer, "v4") == 0){ // se o protocolo for IPV4
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage; // Aloco minha struct com o tamanho correspondente IPV4
        addr4->sin_family = AF_INET; //IPV4
        addr4->sin_addr.s_addr = INADDR_ANY; //Aceito qualquer endereço que o computador tenha na interface de rede dele!
        addr4->sin_port = port; //Porta em que o servidor vai se comunicar com os clientes
        return 0;
    }
    else if (strcmp(portServer, "v6") == 0){ // se o protocolo for IPV6
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