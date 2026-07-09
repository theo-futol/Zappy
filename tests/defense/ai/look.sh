#!/usr/bin/env bash
# AI command: Look
# Returns the content of every tile in the player's field of vision.
# Server response: "[tile0, tile1, ...]"  — a bracket-delimited list
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Look"

zappy_ai_connect
zappy_send "Look"
sleep "$WAIT_7TU"
resp=$(zappy_read)

note "response: $resp"
check_regex "Look — field of vision" "$resp" '^\[.*\]$'
zappy_exit
