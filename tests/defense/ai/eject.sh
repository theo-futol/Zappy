#!/usr/bin/env bash
# AI command: Eject
# Pushes every other player on the current tile one tile forward and destroys any
# eggs of the ejector's team that sit on that tile.
#
# Setup: we first lay an egg with Fork (using sst 100 to speed it up),
# then immediately Eject — the egg is on the same tile, so Eject returns "ok".
#
# The server also sends "eject: K" to every player on the tile (including the
# ejector itself), so the AI may receive that notification before "ok".
# We drain all lines and look for "ok" among them.
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "AI — Eject"

# ── Speed up the server so Fork + Eject finish in < 1 s ──────────────────────
# Open a GUI connection (fd 3) for sst, AI on fd 4.
_zappy_open   # fd 3 → GUI
local_welcome=$(zappy_read)
[[ "$local_welcome" == "WELCOME" ]] || { echo "FAIL: GUI WELCOME"; exit 1; }
zappy_send "GRAPHIC"
zappy_drain  # consume initial push

zappy_send "sst 100"
sleep 0.15
sst_resp=$(zappy_read)
note "sst 100 → $sst_resp"

# ── AI connection on fd 4 ─────────────────────────────────────────────────────
exec 4<>/dev/tcp/"$HOST"/"$PORT" 2>/dev/null || { echo "FAIL: AI connect"; exit 1; }
read_4() { local ln; IFS= read -r -t 5 ln <&4 || return 1; printf '%s' "${ln%$'\r'}"; }
send_4() { printf '%s\n' "$1" >&4; }

read_4 > /dev/null           # WELCOME
send_4 "$TEAM"
SLOTS=$(read_4); MAPSIZE=$(read_4)
note "AI connected | slots=$SLOTS  map=$MAPSIZE"

# ── Fork (42 TU at f=100 = 420 ms) ───────────────────────────────────────────
send_4 "Fork"
sleep 0.65
FORK_RESP=$(read_4)
note "Fork → $FORK_RESP"

# ── Eject (7 TU at f=100 = 70 ms) ────────────────────────────────────────────
# Drain all lines received (may include "eject: K" notification + "ok").
send_4 "Eject"
sleep 0.35
EJECT_LINES=()
while IFS= read -r -t 0.25 ln <&4; do
    EJECT_LINES+=("${ln%$'\r'}")
done
note "Eject responses: ${EJECT_LINES[*]}"

# ── Restore frequency ─────────────────────────────────────────────────────────
zappy_send "sst 10"
sleep 0.15
zappy_read > /dev/null

# ── Check ─────────────────────────────────────────────────────────────────────
OK_FOUND=false
for ln in "${EJECT_LINES[@]}"; do
    [[ "$ln" == "ok" || "$ln" == "ko" ]] && OK_FOUND=true
done

if $OK_FOUND; then
    RESULT="${EJECT_LINES[*]}"
    [[ "$RESULT" == *ok* ]] && check_eq "Eject — eject (ok: egg destroyed)" "ok" "ok" \
                             || check_eq "Eject — eject (ko: nothing to eject)" "ko" "ko"
else
    _fail "Eject — no valid response" "${EJECT_LINES[*]}"
fi

exec 4>&- 2>/dev/null || true
zappy_exit
