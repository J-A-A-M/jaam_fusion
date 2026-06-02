#!/bin/bash
set -euo pipefail

echo "JAAM WIKI"

# Build Docker image (docs are built inside the multistage Dockerfile).
# --load makes the image available in the local daemon so the docker run
# below works even when the active buildx builder uses the docker-container
# driver (which otherwise keeps the result only in the build cache).
echo "Building Docker image..."
docker buildx build --load -t jaam_wiki -f deploy/wiki/Dockerfile .

# Stop and remove old container
echo "Stopping old container..."
docker stop jaam_wiki || true
docker rm jaam_wiki || true

# Deploy new container
echo "Deploying new container..."
docker run --name jaam_wiki \
    --restart unless-stopped \
    --network=jaam \
    -d \
    jaam_wiki

echo "Container deployed successfully!"
