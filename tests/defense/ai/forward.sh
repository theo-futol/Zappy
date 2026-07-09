#!/usr/bin/env bash
# AI command: Forward
# The player moves one tile forward in its facing direction.
# Server response: "ok"  (always succeeds for a living player)
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Forward"

zappy_ai_connect
zappy_send "Forward"
sleep "$WAIT_7TU"
resp=$(zappy_read)

check_eq "Forward — move one tile forward" "$resp" "ok"
zappy_exit
