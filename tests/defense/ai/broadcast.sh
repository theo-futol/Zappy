#!/usr/bin/env bash
# AI command: Broadcast
# Sends a message to every connected player.
# Every other player receives "message K, <text>" where K is the direction.
# The broadcaster receives "ok".
# Note: the server parser splits on whitespace, so the message must be one word.
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Broadcast"

zappy_ai_connect
zappy_send "Broadcast hello"
sleep "$WAIT_7TU"
resp=$(zappy_read)

check_eq "Broadcast — send message to all players" "$resp" "ok"
zappy_exit
