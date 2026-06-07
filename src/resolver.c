// resolver.c
#include "resolver.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

int resolve_target(const char *hostname, char *ip_buffer, int buf_size) {
    // Try as IP first
    struct in_addr addr;
    if (inet_pton(AF_INET, hostname, &addr) == 1) {
        strncpy(ip_buffer, hostname, buf_size - 1);
        return 0;
    }
    
    // DNS resolution
    struct hostent *he = gethostbyname(hostname);
    if (he == NULL) return -1;
    
    strncpy(ip_buffer, inet_ntoa(*(struct in_addr *)he->h_addr_list[0]), buf_size - 1);
    return 0;
}
