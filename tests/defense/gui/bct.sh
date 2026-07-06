#!/usr/bin/env bash
# GUI command: bct X Y
# Requests the resource content of tile (X, Y).
# Server response: "bct X Y q0 q1 q2 q3 q4 q5 q6"
#   where q0–q6 are the counts for food, linemate, deraumere, sibur,
#   mendiane, phiras, and thystame respectively.
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "GUI — bct X Y"

zappy_gui_connect
zappy_send "bct 0 0"
sleep 0.10
resp=$(zappy_read)

note "response: $resp"
check_prefix "bct X Y — tile content" "$resp" "bct 0 0"
zappy_exit
