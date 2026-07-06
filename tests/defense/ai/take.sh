#!/usr/bin/env bash
# AI command: Take
# Picks up one unit of a resource from the current tile.
# Setup: first drop food with Set (player starts with 10 food), then pick it back up.
# Server response: "ok"
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Take"

zappy_ai_connect

# Drop food onto the tile so we have something to pick up.
zappy_send "Set food"
sleep "$WAIT_7TU"
set_resp=$(zappy_read)
note "Set food → $set_resp"

# Pick the food back up.
zappy_send "Take food"
sleep "$WAIT_7TU"
resp=$(zappy_read)

check_eq "Take food — collect item from tile" "$resp" "ok"
zappy_exit
