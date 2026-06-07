#!/usr/bin/env python3
"""
Scrape public SOCKS4/5 proxies for attack distribution
FOR AUTHORIZED PENETRATION TESTING ONLY
"""

import urllib.request
import re
import os
import sys
import argparse
import random


PROXY_SOURCES = [
    "https://api.proxyscrape.com/?request=getproxies&proxytype=socks4&timeout=10000",
    "https://api.proxyscrape.com/?request=getproxies&proxytype=socks5&timeout=10000",
    "https://raw.githubusercontent.com/roosterkid/openproxylist/main/SOCKS4.txt",
    "https://raw.githubusercontent.com/roosterkid/openproxylist/main/SOCKS5.txt",
    "https://raw.githubusercontent.com/TheSpeedX/PROXY-List/master/socks4.txt",
    "https://raw.githubusercontent.com/TheSpeedX/PROXY-List/master/socks5.txt",
    "https://raw.githubusercontent.com/ShiftyTR/Proxy-List/master/socks4.txt",
    "https://raw.githubusercontent.com/ShiftyTR/Proxy-List/master/socks5.txt",
    "https://raw.githubusercontent.com/clarketm/proxy-list/master/proxy-list-raw.txt",
    "https://www.proxy-list.download/api/v1/get?type=socks4",
    "https://www.proxy-list.download/api/v1/get?type=socks5",
]

IP_PORT_PATTERN = re.compile(r'(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(\d+)')


def scrape_sources(sources, timeout=5):
    all_proxies = set()
    
    for url in sources:
        try:
            req = urllib.request.Request(url, headers={
                'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36'
            })
            data = urllib.request.urlopen(req, timeout=timeout).read().decode('utf-8', errors='ignore')
            
            matches = IP_PORT_PATTERN.findall(data)
            for ip, port in matches:
                all_proxies.add((ip, port))
            
            print(f"[+] {url.split('/')[2]}: {len(matches)} proxies")
        except Exception as e:
            print(f"[-] {url.split('/')[2]}: {str(e)[:30]}")
    
    return list(all_proxies)


def validate_proxy(ip, port, timeout=3):
    """Quick check if proxy is alive"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        s.connect((ip, int(port)))
        s.close()
        return True
    except:
        return False


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Proxy scraper for penetration testing")
    parser.add_argument("-o", "--output", default="config/proxy_list.txt", help="Output file")
    parser.add_argument("-c", "--count", type=int, default=500, help="Max proxies to save")
    parser.add_argument("--validate", action="store_true", help="Validate proxies (slow)")
    args = parser.parse_args()
    
    print("[*] Scraping proxies from public sources...\n")
    proxies = scrape_sources(PROXY_SOURCES)
    
    print(f"\n[*] Total unique proxies found: {len(proxies)}")
    
    if args.validate:
        print("[*] Validating proxies (this may take a while)...")
        valid = []
        for i, (ip, port) in enumerate(proxies[:args.count * 2]):
            if validate_proxy(ip, port):
                valid.append((ip, port))
            if i % 50 == 0:
                print(f"    Checked {i+1}/{min(len(proxies), args.count*2)}...", end='\r')
        proxies = valid
        print(f"\n[*] Valid proxies: {len(proxies)}")
    
    # Save
    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    with open(args.output, 'w') as f:
        for ip, port in proxies[:args.count]:
            f.write(f"{ip}:{port}:socks4\n")
    
    print(f"[*] Saved {min(len(proxies), args.count)} proxies to {args.output}")
