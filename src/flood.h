#ifndef FLOOD_H
#define FLOOD_H

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>

// Attack modes
typedef enum {
    MODE_SYN = 0,
    MODE_UDP,
    MODE_TCP,
    MODE_ACK,
    MODE_UNKNOWN
} attack_mode_t;

// Attack configuration
typedef struct {
    char target[MAX_TARGET_LEN];
    char target_ip[MAX_IP_LEN];
    int port;
    int packet_size;
    int duration;
    attack_mode_t mode;
    int num_threads;
    volatile int running;
} attack_config_t;

// Per-thread data
typedef struct {
    attack_config_t *config;
    int thread_id;
    unsigned long long packets_sent;
    unsigned long long bytes_sent;
} thread_data_t;

// Global stats
typedef struct {
    unsigned long long total_packets;
    unsigned long long total_bytes;
    double start_time;
    double elapsed;
    pthread_mutex_t lock;
} global_stats_t;

// Function prototypes
void *syn_flood_thread(void *arg);
void *udp_flood_thread(void *arg);
void *tcp_flood_thread(void *arg);
void *ack_flood_thread(void *arg);
void print_stats(global_stats_t *stats, attack_config_t *config);
void sigint_handler(int sig);
unsigned short checksum(void *b, int len);
double get_time_sec(void);

#endif /* FLOOD_H */
