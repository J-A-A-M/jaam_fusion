#!/bin/bash
set -euo pipefail

# Host-side deploy step: (re)create the wiki container from a prebuilt image.
# CI builds a clean nginx + static-files image and delivers it to this host
# (either via GHCR pull or `docker load`); this just swaps the running
# container over to it. No build happens on the host.
IMAGE="${1:?usage: redeploy_wiki.sh <image-ref>}"

echo "Deploying JAAM WIKI from image ${IMAGE}"

docker rm -f jaam_wiki 2>/dev/null || true
docker run --name jaam_wiki \
    --restart unless-stopped \
    --network=jaam \
    -d "$IMAGE"

# Drop the now-dangling previous image.
docker image prune -f

echo "Container deployed."
