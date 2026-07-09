#!/usr/bin/env bash
# AI command: Inventory
# Returns the quantity of each resource in the player's inventory.
# Server response: "[food N, linemate N, ...]"  (cost: 1 TU)
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Inventory"

zappy_ai_connect
zappy_send "Inventory"
sleep "$WAIT_1TU"
resp=$(zappy_read)

note "response: $resp"
check_regex "Inventory — player resources" "$resp" '^\[food [0-9]+'
zappy_exit
