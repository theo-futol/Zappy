#!/usr/bin/env bash
# GUI command: pin #n
# Requests the inventory and position of player #n.
# Server response: "pin #n X Y q0 q1 q2 q3 q4 q5 q6"
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "GUI — pin #n"

# ── Start an AI player (fd 4) ─────────────────────────────────────────────────
exec 4<>/dev/tcp/"$HOST"/"$PORT" 2>/dev/null || { echo "FAIL: AI connect"; exit 1; }
IFS= read -r -t 3 _w <&4
printf '%s\n' "$TEAM" >&4
IFS= read -r -t 3 _s <&4
IFS= read -r -t 3 _m <&4
note "AI connected | slots=${_s%$'\r'}  map=${_m%$'\r'}"

sleep 0.20

# ── GUI (fd 3) ────────────────────────────────────────────────────────────────
zappy_gui_connect
PID="${GUI_PLAYER_ID:-1}"
note "querying player #$PID"

zappy_send "pin $PID"
sleep 0.10
resp=$(zappy_read)
note "response: $resp"
check_prefix "pin #n — player inventory" "$resp" "pin $PID "

exec 4>&- 2>/dev/null || true
zappy_exit
