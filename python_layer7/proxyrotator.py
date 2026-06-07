#!/usr/bin/env python3
"""
Proxy rotator for distributed HTTP attacks
FOR AUTHORIZED PENETRATION TESTING ONLY
"""

import random
import threading
import socket
import time


class ProxyRotator:
    def __init__(self, proxy_list_file=None):
        self.proxies = []
        self.blacklist = set()
        self.lock = threading.Lock()
        
        if proxy_list_file:
            self.load(proxy_list_file)
    
    def load(self, filename):
        with open(filename, 'r') as f:
            for line in f:
                line = line.strip()
                if line and ':' in line:
                    parts = line.split(':')
                    try:
                        self.proxies.append({
                            'ip': parts[0],
                            'port': int(parts[1]),
                            'protocol': parts[2] if len(parts) > 2 else 'socks4',
                            'failures': 0,
                            'last_used': 0
                        })
                    except:
                        pass
        print(f"[*] Loaded {len(self.proxies)} proxies")
    
    def get_random(self):
        with self.lock:
            available = [p for p in self.proxies 
                        if p['ip'] not in self.blacklist and 
                        p['failures'] < 3]
            if not available:
                return None
            return random.choice(available)
    
    def mark_failed(self, proxy):
        with self.lock:
            proxy['failures'] += 1
            if proxy['failures'] >= 3:
                self.blacklist.add(proxy['ip'])
    
    def get_working_count(self):
        with self.lock:
            return len([p for p in self.proxies if p['ip'] not in self.blacklist])
