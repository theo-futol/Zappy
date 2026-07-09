#!/usr/bin/env bash
# AI command: Set
# Drops one unit of a resource from the player's inventory onto the current tile.
# Players spawn with 10 food units, so "Set food" always succeeds.
# Server response: "ok"
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Set"

zappy_ai_connect
zappy_send "Set food"
sleep "$WAIT_7TU"
resp=$(zappy_read)

check_eq "Set food — drop item on tile" "$resp" "ok"
zappy_exit
