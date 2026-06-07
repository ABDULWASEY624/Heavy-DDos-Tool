// cloudflare.h
#ifndef CLOUDFLARE_H
#define CLOUDFLARE_H

// HTTP headers for Cloudflare bypass
#define CF_BYPASS_HEADERS \
    "X-Forwarded-For: %s\r\n" \
    "X-Real-IP: %s\r\n" \
    "CF-Connecting-IP: %s\r\n" \
    "X-Originating-IP: %s\r\n" \
    "X-Remote-IP: %s\r\n" \
    "X-Remote-Addr: %s\r\n" \
    "X-Client-IP: %s\r\n" \
    "Forwarded: for=%s;proto=http\r\n"

#define HTTP_HEADERS \
    "User-Agent: %s\r\n" \
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n" \
    "Accept-Language: en-US,en;q=0.9\r\n" \
    "Accept-Encoding: gzip, deflate, br\r\n" \
    "Connection: keep-alive\r\n" \
    "Upgrade-Insecure-Requests: 1\r\n" \
    "Sec-Fetch-Dest: document\r\n" \
    "Sec-Fetch-Mode: navigate\r\n" \
    "Sec-Fetch-Site: none\r\n" \
    "Sec-Fetch-User: ?1\r\n" \
    "Cache-Control: no-cache\r\n" \
    "Pragma: no-cache\r\n"

#define NUM_USER_AGENTS 20

extern const char *user_agents[NUM_USER_AGENTS];
char *generate_cf_headers(void);
char *generate_random_ua(void);
char *generate_random_ip(void);

#endif
