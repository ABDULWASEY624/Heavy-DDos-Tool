# HighSpeed DDoS - Multi-Vector Attack Suite

**FOR AUTHORIZED PENETRATION TESTING AND SECURITY RESEARCH ONLY**

A high-performance, multi-vector denial-of-service simulation tool designed for professional security testers. Combines C-based raw packet flooding (L3/L4) with Python-based Layer 7 application attacks, including Cloudflare bypass capabilities.

## ⚠️ Legal Notice

This tool is intended **only** for:
- Authorized penetration testing on systems you own or have explicit written permission to test
- Security research in isolated lab environments
- Educational purposes

**Unauthorized use is illegal.** You are solely responsible for complying with all applicable laws.

## Architecture

## Features

### C Engine (Raw Performance)
- **SYN Flood** - TCP SYN packets with spoofed source IPs
- **UDP Flood** - High-volume UDP datagrams
- **TCP Flood** - Full TCP connection flood
- **ACK Flood** - ACK packet storm
- **Multi-threaded** - One thread per CPU core for maximum throughput
- **Custom packet size** - Configurable payload size (up to 65,535 bytes per packet)
- **Source IP randomization** - Spoofed source addresses

### Python Engine (Layer 7 + Cloudflare Bypass)
- **HTTP/HTTPS Flood** - Random paths, headers, User-Agents
- **HTTP/2 Rapid Reset** - Modern HTTP/2 attack vector
- **Slowloris** - Slow connection depletion
- **Cloudflare Bypass**:
  - `CF-Connecting-IP` / `X-Forwarded-For` spoofing
  - TLS fingerprint randomization
  - Browser User-Agent rotation
  - Referer chain spoofing
  - Cache buster URLs
  - JS challenge detection
- **Proxy rotation** - SOCKS4/5 proxy support

## Performance Expectations

On a single VPS with **100 KB packets** over **120 seconds**:

| VPS Bandwidth | C Engine | Python Engine |
|--------------|----------|---------------|
| 1 Gbps       | ~15 GB   | ~500-800 MB   |
| 2.5 Gbps     | ~37 GB   | ~600 MB - 1 GB|
| 5 Gbps       | ~75 GB   | ~700 MB - 1 GB|
| 10 Gbps      | ~150 GB  | ~800 MB - 1.2 GB|

## Installation

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential git python3 python3-pip \
                    libcurl4-openssl-dev libssl-dev libpthread-stubs0-dev

# Required Python packages
pip3 install -r requirements.txt

