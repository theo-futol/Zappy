#!/usr/bin/env bash
# GUI command: ppo #n
# Requests the position and orientation of player #n.
# Server response: "ppo #n X Y O"  (O = orientation 1–4)
#
# This test connects an AI player first (fd 4), then the GUI (fd 3).
# The server includes the player's pnw line in the GUI's initial push,
# which is used to recover the player ID.
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "GUI — ppo #n"

# ── Start an AI player (fd 4) ─────────────────────────────────────────────────
exec 4<>/dev/tcp/"$HOST"/"$PORT" 2>/dev/null || { echo "FAIL: AI connect"; exit 1; }
IFS= read -r -t 3 _w <&4   # WELCOME
printf '%s\n' "$TEAM" >&4
IFS= read -r -t 3 _s <&4   # slots
IFS= read -r -t 3 _m <&4   # mapsize
note "AI connected | slots=${_s%$'\r'}  map=${_m%$'\r'}"

sleep 0.20  # let the server register the player

# ── GUI (fd 3) — reads pnw for the AI player in its initial push ──────────────
zappy_gui_connect
PID="${GUI_PLAYER_ID:-1}"
note "querying player #$PID"

# ── Query ─────────────────────────────────────────────────────────────────────
zappy_send "ppo $PID"
sleep 0.10
resp=$(zappy_read)
note "response: $resp"
check_regex "ppo #n — player position and orientation" "$resp" "^ppo $PID [0-9]+ [0-9]+ [0-9]+$"

exec 4>&- 2>/dev/null || true
zappy_exit
