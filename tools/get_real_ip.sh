#!/bin/bash
# Quick origin IP discovery using multiple techniques
# Usage: ./get_real_ip.sh example.com

TARGET="$1"

if [ -z "$TARGET" ]; then
    echo "Usage: $0 <domain>"
    exit 1
fi

echo "=== Checking $TARGET ==="

# Check if behind Cloudflare
cf=$(dig +short TXT _cloudflare.$TARGET 2>/dev/null)
if [ -n "$cf" ]; then
    echo "[!] $TARGET is using Cloudflare"
fi

# Try direct A record
echo "A record: $(dig +short A $TARGET)"

# Try CloudFlare-sourced IPs from historical data using common patterns
echo ""
echo "Checking historical DNS and reverse lookups..."
echo "Visit: https://securitytrails.com/domain/$TARGET/dns"
echo "Visit: https://viewdns.info/iphistory/?domain=$TARGET"
echo "Visit: https://www.criminalip.io/domain/report/$TARGET"
echo ""
echo "To test if an IP is the origin:"
echo "  curl -H \"Host: $TARGET\" https://<ip>/"
echo "  curl -H \"Host: $TARGET\" http://<ip>/"
