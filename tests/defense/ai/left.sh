#!/usr/bin/env bash
# AI command: Left
# The player turns 90° counter-clockwise in place.
# Server response: "ok"
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Left"

zappy_ai_connect
zappy_send "Left"
sleep "$WAIT_7TU"
resp=$(zappy_read)

check_eq "Left — turn 90° counter-clockwise" "$resp" "ok"
zappy_exit
