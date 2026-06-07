#!/bin/bash
# Find real origin IP behind Cloudflare
# Usage: ./find_origin.sh example.com

TARGET="$1"

if [ -z "$TARGET" ]; then
    echo "Usage: $0 <domain>"
    echo "Example: $0 example.com"
    exit 1
fi

echo "==========================================="
echo "  Origin IP Discovery for $TARGET"
echo "==========================================="
echo ""

# 1. Direct DNS resolution
echo "[1] Direct A record:"
dig +short A "$TARGET"
echo ""

# 2. Historical DNS (SecurityTrails - requires API)
echo "[2] Censys/Shodan suggested search queries:"
echo "    https://search.censys.io/search?resource=hosts&q=$TARGET"
echo "    https://www.shodan.io/search?query=hostname%3A$TARGET"
echo "    https://securitytrails.com/domain/$TARGET/dns"
echo ""

# 3. Check common subdomains for exposed origins
echo "[3] Checking subdomains for origin IPs:"
SUBDOMAINS=("direct" "origin" "mail" "webmail" "admin" "cdn" "static" 
            "img" "images" "www2" "test" "dev" "staging" "api" "vpn"
            "ssh" "ftp" "ns1" "ns2" "mx" "smtp" "pop3" "remote")

for sub in "${SUBDOMAINS[@]}"; do
    ip=$(dig +short "$sub.$TARGET" A 2>/dev/null)
    if [ -n "$ip" ]; then
        echo "  $sub.$TARGET -> $ip"
    fi
done
echo ""

# 4. Check MX records (often reveal real IP)
echo "[4] MX records (may reveal origin):"
dig +short MX "$TARGET" | while read line; do
    mx_host=$(echo "$line" | awk '{print $2}')
    mx_ip=$(dig +short "$mx_host" A 2>/dev/null)
    if [ -n "$mx_ip" ]; then
        echo "  $mx_host -> $mx_ip"
    fi
done
echo ""

# 5. Check NS records
echo "[5] NS records:"
dig +short NS "$TARGET" | while read ns; do
    ns_ip=$(dig +short "$ns" A 2>/dev/null)
    if [ -n "$ns_ip" ]; then
        echo "  $ns -> $ns_ip"
    fi
done
echo ""

# 6. Check TXT records for clues
echo "[6] TXT records:"
dig +short TXT "$TARGET" | head -5
echo ""

# 7. CNAME check
echo "[7] CNAME:"
dig +short CNAME "$TARGET"
echo ""

echo "==========================================="
echo "  To discover origin IP, also try:"
echo "==========================================="
echo "  - Certificate Transparency: https://crt.sh/?q=%25.$TARGET"
echo "  - DNS history: https://securitytrails.com"
echo "  - Shodan: https://www.shodan.io/search?query=hostname%3A$TARGET"
echo "  - Censys: https://search.censys.io"
echo ""
echo "  If you find the origin IP:"
echo "  # Verify it's the real server:"
echo "  curl -H 'Host: $TARGET' https://<origin_ip>/"
echo ""
