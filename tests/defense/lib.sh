#!/usr/bin/env bash
# Shared helpers for Zappy defense tests.
# Source this file; do NOT execute it directly.
#
# Usage in a test script:
#   source "$(dirname "$0")/../lib.sh"

HOST="${ZAPPY_HOST:-127.0.0.1}"
PORT="${ZAPPY_PORT:-4242}"
TEAM="${ZAPPY_TEAM:-team1}"

# ── Timing constants at server default f=10 ────────────────────────────────────
# Each constant adds a 200 ms safety margin over the theoretical execution time.
WAIT_0TU=0.25   # Connect_nbr (0 TU)
WAIT_1TU=0.35   # Inventory   (1 TU  = 100 ms)
WAIT_7TU=0.90   # Forward / Right / Left / Look / Broadcast / Set / Take / Eject (7 TU = 700 ms)
WAIT_42TU=4.50  # Fork        (42 TU = 4 200 ms)

PASS_COUNT=0
FAIL_COUNT=0

# ── fd 3 connection helpers ────────────────────────────────────────────────────

_zappy_open() {
    exec 3<>/dev/tcp/"$HOST"/"$PORT" 2>/dev/null || {
        printf 'FAIL  cannot connect to %s:%s\n' "$HOST" "$PORT"
        exit 1
    }
}

# Read one '\n'-terminated line from fd 3 (strips trailing CR).
zappy_read() {
    local _line
    IFS= read -r -t "${1:-5}" _line <&3 || return 1
    printf '%s' "${_line%$'\r'}"
}

# Send one line to fd 3.
zappy_send() {
    printf '%s\n' "$1" >&3
}

# Read all available lines from fd 3 into the global LINES array;
# stops after a 250 ms silence window.
zappy_drain() {
    LINES=()
    local _line
    while IFS= read -r -t 0.25 _line <&3; do
        LINES+=("${_line%$'\r'}")
    done
}

# Close fd 3.
zappy_close() {
    exec 3>&- 2>/dev/null || true
}

# ── Handshake helpers ──────────────────────────────────────────────────────────

zappy_ai_connect() {
    _zappy_open
    local welcome
    welcome=$(zappy_read)
    [[ "$welcome" == "WELCOME" ]] || { printf 'FAIL: expected WELCOME, got %s\n' "$welcome"; exit 1; }
    zappy_send "${1:-$TEAM}"
    zappy_read > /dev/null  # slots
    zappy_read > /dev/null  # map size
}

zappy_gui_connect() {
    _zappy_open
    local welcome
    welcome=$(zappy_read)
    [[ "$welcome" == "WELCOME" ]] || { printf 'FAIL: expected WELCOME, got %s\n' "$welcome"; exit 1; }
    zappy_send "GRAPHIC"
    zappy_drain
    GUI_INIT=("${LINES[@]}")
    GUI_PLAYER_ID=""
    local ln
    for ln in "${GUI_INIT[@]}"; do
        if [[ "$ln" =~ ^pnw\ ([0-9]+) ]]; then
            GUI_PLAYER_ID="${BASH_REMATCH[1]}"
            break
        fi
    done
}

# ── Assertion helpers ──────────────────────────────────────────────────────────

_pass() { PASS_COUNT=$((PASS_COUNT + 1)); printf '%s\n' "$2"; }
_fail() { FAIL_COUNT=$((FAIL_COUNT + 1)); printf 'FAIL  %s\n' "$1: $2"; }

check_eq()     { [[ "$2" == "$3"  ]] && _pass "$1" "$2" || _fail "$1" "got='$2' expected='$3'"; }
check_prefix() { [[ "$2" == "$3"* ]] && _pass "$1" "$2" || _fail "$1" "got='$2' expected prefix='$3'"; }
check_regex()  { [[ "$2" =~ $3   ]] && _pass "$1" "$2" || _fail "$1" "got='$2' pattern='$3'"; }

check_in() {
    local label="$1" got="$2"; shift 2
    local v
    for v in "$@"; do [[ "$got" == "$v" ]] && { _pass "$label" "$got"; return; }; done
    _fail "$label" "got='$got' expected one of: $*"
}

check_nonempty() { [[ -n "$2" ]] && _pass "$1" "$2" || _fail "$1" "(empty)"; }

# ── Output helpers ─────────────────────────────────────────────────────────────

banner() { printf '\n=== %s ===\n' "$1"; }

note() { :; }

zappy_exit() {
    zappy_close
    [[ $FAIL_COUNT -eq 0 ]] && exit 0 || exit 1
}
