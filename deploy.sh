#!/usr/bin/env bash
# Deploy the Zappy server to the VPS: sync sources, rebuild, restart the service.
# Usage: ./deploy.sh
set -euo pipefail

VPS_USER=ubuntu
VPS_HOST=51.255.206.237
REMOTE_DIR='~/zappy'

SSH="ssh -o BatchMode=yes -o StrictHostKeyChecking=accept-new ${VPS_USER}@${VPS_HOST}"

echo ">> Syncing server sources to ${VPS_HOST}..."
rsync -az --relative --exclude '*.o' -e "ssh -o BatchMode=yes -o StrictHostKeyChecking=accept-new" \
      ./Makefile ./src/server "${VPS_USER}@${VPS_HOST}:${REMOTE_DIR}/"

echo ">> Building on the VPS (g++ linker; the Makefile hardcodes clang++)..."
# shellcheck disable=SC2016
$SSH 'cd ~/zappy && make -C src/server CC=g++ BINARY_LOCATION=$HOME/zappy/zappy_server'

echo ">> Restarting the service..."
$SSH 'sudo systemctl restart zappy && systemctl is-active zappy && ss -tlnp | grep 4242'

echo ">> Done. Server live on ${VPS_HOST}:4242"
