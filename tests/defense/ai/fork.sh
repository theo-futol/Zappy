#!/usr/bin/env bash
# AI command: Fork
# The player lays an egg on the current tile, adding one connection slot to the team.
# Cost: 42 TU = 4 200 ms at f=10  →  this test takes ~4.5 s.
# Server response: "ok"
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Fork  (42 TU at f=10 ≈ 4.2 s)"

zappy_ai_connect
note "waiting 4.2 s for Fork to complete..."
zappy_send "Fork"
sleep "$WAIT_42TU"
resp=$(zappy_read)

check_eq "Fork — lay an egg on current tile" "$resp" "ok"
zappy_exit
