#!/usr/bin/env bash
# AI command: Connect_nbr
# Returns the number of free connection slots left for the player's team.
# Server response: a plain integer  (cost: 0 TU, executes immediately)
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Connect_nbr"

zappy_ai_connect
zappy_send "Connect_nbr"
sleep "$WAIT_0TU"
resp=$(zappy_read)

note "response: $resp"
check_regex "Connect_nbr — free team slots" "$resp" '^[0-9]+$'
zappy_exit
