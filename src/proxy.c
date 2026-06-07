// proxy.c
#include "proxy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int load_proxies(const char *filename, proxy_entry_t **proxies) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    
    int capacity = 256;
    int count = 0;
    *proxies = malloc(sizeof(proxy_entry_t) * capacity);
    
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (count >= capacity) {
            capacity *= 2;
            *proxies = realloc(*proxies, sizeof(proxy_entry_t) * capacity);
        }
        
        // Format: ip:port:protocol
        char ip[64], proto[8];
        int port;
        if (sscanf(line, "%[^:]:%d:%s", ip, &port, proto) >= 2) {
            strncpy((*proxies)[count].ip, ip, 63);
            (*proxies)[count].port = port;
            strncpy((*proxies)[count].protocol, "socks4", 7);
            count++;
        }
    }
    
    fclose(f);
    return count;
}

void free_proxies(proxy_entry_t *proxies, int count) {
    (void)count;
    free(proxies);
}

proxy_entry_t get_random_proxy(proxy_entry_t *proxies, int count) {
    if (count == 0) {
        proxy_entry_t empty = {"0.0.0.0", 0, "none"};
        return empty;
    }
    return proxies[rand() % count];
}
