#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// HighSpeed DDoS - Configuration
// ============================================================

// Thread control
#define NUM_THREADS 0            // 0 = auto-detect (CPU cores)

// Network tuning
#define SOURCE_PORT_MIN 1024     // Minimum spoofed source port
#define SOURCE_PORT_MAX 65535    // Maximum spoofed source port
#define TTL_VALUE 64             // IP TTL
#define WINDOW_SIZE 65535        // TCP window size

// Packet generation
#define DEFAULT_PACKET_SIZE 99999   // Default payload size (bytes)
#define PACKET_FILL_CHAR 'A'        // Character to fill unused payload

// SYN flood
#define SYN_FLOOD 1
#define UDP_FLOOD 1
#define TCP_FLOOD 1
#define ACK_FLOOD 1

// Performance
#define BATCH_SIZE 100           // Packets per batch send
#define MAX_SOCKETS 1000         // Max concurrent sockets (TCP mode)

// Stats refresh interval (seconds)
#define STATS_INTERVAL 1

// Max URL for config
#define MAX_TARGET_LEN 256
#define MAX_IP_LEN 64

#endif /* CONFIG_H */
