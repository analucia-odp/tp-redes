#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <string.h>

int buildStorage(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
void parseAddressToString(const struct sockaddr *addr, char *str, size_t strsize);
int buildStorageServer(const char *portServer, const char* portClient, struct sockaddr_storage *storage);
void logexit (const char *msg);