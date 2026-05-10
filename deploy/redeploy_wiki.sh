#!/bin/bash
set -euo pipefail

echo "JAAM WIKI"

# Build documentation
echo "Installing Python dependencies..."
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt --quiet

echo "Building documentation..."
.venv/bin/mkdocs build --clean

# Build Docker image
echo "Building Docker image..."
docker build -t jaam_wiki -f deploy/wiki/Dockerfile .

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
