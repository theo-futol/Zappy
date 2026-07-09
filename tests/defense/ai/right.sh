#!/usr/bin/env bash
# AI command: Right
# The player turns 90° clockwise in place.
# Server response: "ok"
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Right"

zappy_ai_connect
zappy_send "Right"
sleep "$WAIT_7TU"
resp=$(zappy_read)

check_eq "Right — turn 90° clockwise" "$resp" "ok"
zappy_exit
