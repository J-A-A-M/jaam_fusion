#!/bin/bash
set -euo pipefail

# Host-side deploy step. The static docs are rsynced to $HTML_DIR by CI;
# this just (re)creates a stock nginx container that serves them read-only on
# the jaam network. No build happens on the host.
HTML_DIR="${1:?usage: redeploy_wiki.sh <html-dir>}"

echo "Serving JAAM WIKI from ${HTML_DIR}"

docker pull nginx:alpine
docker rm -f jaam_wiki 2>/dev/null || true
docker run --name jaam_wiki \
    --restart unless-stopped \
    --network=jaam \
    -v "${HTML_DIR}:/usr/share/nginx/html:ro" \
    -d nginx:alpine

# Drop the now-dangling old nginx image left by the pull above.
docker image prune -f

echo "Container deployed."
