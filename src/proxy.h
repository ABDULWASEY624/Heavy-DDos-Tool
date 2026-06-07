// proxy.h
#ifndef PROXY_H
#define PROXY_H

typedef struct {
    char ip[64];
    int port;
    char protocol[8];  // socks4, socks5, http
} proxy_entry_t;

int load_proxies(const char *filename, proxy_entry_t **proxies);
void free_proxies(proxy_entry_t *proxies, int count);
proxy_entry_t get_random_proxy(proxy_entry_t *proxies, int count);

#endif
