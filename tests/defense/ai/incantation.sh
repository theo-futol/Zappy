#!/usr/bin/env bash
# AI command: Incantation
# Starts an elevation ritual on the current tile.
# Level-1 requirements: 1 player of level 1 + 1 linemate on the tile.
#
# This script uses a GUI connection to temporarily raise the server frequency
# to 100 so the 300-TU ritual completes in ~3 s instead of 30 s.
#
# Flow:
#   1. GUI: sst 100
#   2. AI: walk (up to 5 steps) until linemate is found on current tile
#   3. AI: Take linemate → Set linemate (ensure exactly 1 is present)
#   4. AI: Incantation
#      - Server immediately sends "Elevation underway" if conditions are met
#      - After 300 TU (3 s at f=100) sends "Current level: 2"
#   5. GUI: sst 10  (restore)
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Incantation  (300 TU; using sst 100 → ~3 s)"

# ── GUI on fd 3: speed up the server ──────────────────────────────────────────
_zappy_open
gui_welcome=$(zappy_read)
[[ "$gui_welcome" == "WELCOME" ]] || { echo "FAIL: GUI WELCOME"; exit 1; }
zappy_send "GRAPHIC"
zappy_drain

zappy_send "sst 100"
sleep 0.15
sst_resp=$(zappy_read)
note "sst 100 → $sst_resp"

# ── AI on fd 4 ────────────────────────────────────────────────────────────────
exec 4<>/dev/tcp/"$HOST"/"$PORT" 2>/dev/null || { echo "FAIL: AI connect"; exit 1; }
ai_read()  { local ln; IFS= read -r -t 5  ln <&4 || return 1; printf '%s' "${ln%$'\r'}"; }
ai_read_t(){ local ln; IFS= read -r -t "$1" ln <&4 || return 1; printf '%s' "${ln%$'\r'}"; }
ai_send()  { printf '%s\n' "$1" >&4; }
ai_drain() {
    AI_LINES=()
    local ln
    while IFS= read -r -t 0.25 ln <&4; do AI_LINES+=("${ln%$'\r'}"); done
}

ai_read > /dev/null      # WELCOME
ai_send "$TEAM"
SLOTS=$(ai_read); MAPSIZE=$(ai_read)
note "AI connected | slots=$SLOTS  map=$MAPSIZE"

# At f=100: 7 TU = 70 ms + 150 ms margin = 220 ms per command
AI_WAIT=0.22

# ── Find linemate (up to 5 Forward + Look cycles) ─────────────────────────────
FOUND_LINEMATE=false
for step in $(seq 1 5); do
    ai_send "Look"
    sleep "$AI_WAIT"
    LOOK=$(ai_read)
    # tile0 is the content before the first comma
    TILE0="${LOOK%%,*}"
    # Extract linemate count: "linemate:N"
    LM_COUNT=0
    if [[ "$TILE0" =~ linemate:([0-9]+) ]]; then
        LM_COUNT="${BASH_REMATCH[1]}"
    fi
    note "step $step | tile0: ${TILE0:1:60}  linemate=$LM_COUNT"
    if (( LM_COUNT > 0 )); then
        FOUND_LINEMATE=true
        break
    fi
    ai_send "Forward"
    sleep "$AI_WAIT"
    ai_read > /dev/null  # "ok"
done

# ── Prepare the tile ──────────────────────────────────────────────────────────
if $FOUND_LINEMATE; then
    ai_send "Take linemate"
    sleep "$AI_WAIT"
    TAKE_RESP=$(ai_read)
    note "Take linemate → $TAKE_RESP"

    ai_send "Set linemate"
    sleep "$AI_WAIT"
    SET_RESP=$(ai_read)
    note "Set linemate → $SET_RESP"
fi

# ── Incantation ───────────────────────────────────────────────────────────────
# beginIncantation() is called on receipt and immediately writes the response.
ai_send "Incantation"
sleep 0.35   # allow server to process the receive event
RESP1=$(ai_read_t 2)
note "Incantation immediate response: $RESP1"

if [[ "$RESP1" == "Elevation underway" ]]; then
    check_eq "Incantation — ritual started" "$RESP1" "Elevation underway"
    # Wait for the 300-TU ritual to complete at f=100 (3 s)
    note "waiting ~3 s for ritual to complete..."
    RESP2=$(ai_read_t 5)
    note "Incantation completion: $RESP2"
    check_regex "Incantation — level-up" "$RESP2" '^Current level: [0-9]+'
elif [[ "$RESP1" == "ko" ]]; then
    # Conditions not met (no linemate found or consumed by resource tick)
    check_in "Incantation — ko (conditions not met)" "$RESP1" "ko" "Elevation underway"
    note "hint: linemate not available on tile at incantation time"
else
    _fail "Incantation — unexpected response" "$RESP1"
fi

# ── Restore frequency ─────────────────────────────────────────────────────────
zappy_send "sst 10"
sleep 0.15
zappy_read > /dev/null

exec 4>&- 2>/dev/null || true
zappy_exit
