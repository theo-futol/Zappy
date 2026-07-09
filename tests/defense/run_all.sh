#!/usr/bin/env bash
# Run every defense test script and print a final summary.
#
# Usage:
#   ./tests/defense/run_all.sh [host] [port]
#
# Pre-requisite: the server must already be running with at least -c 25
#   ./zappy_server -p 4242 -x 10 -y 10 -n team1 team2 -c 25 -f 10
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export ZAPPY_HOST="${1:-127.0.0.1}"
export ZAPPY_PORT="${2:-4242}"
export ZAPPY_TEAM="team1"

GREEN='\033[32m'
RED='\033[31m'
YELLOW='\033[33m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m'

TOTAL_PASS=0
TOTAL_FAIL=0
FAILED_SCRIPTS=()

# ── Ordered list of scripts ────────────────────────────────────────────────────
AI_SCRIPTS=(
    ai/forward.sh
    ai/right.sh
    ai/left.sh
    ai/look.sh
    ai/take.sh
    ai/inventory.sh
    ai/set.sh
    ai/connect_nbr.sh
    ai/broadcast.sh
    ai/fork.sh
    ai/eject.sh
    ai/incantation.sh
)

GUI_SCRIPTS=(
    gui/msz.sh
    gui/bct.sh
    gui/mct.sh
    gui/tna.sh
    gui/sgt.sh
    gui/sst.sh
    gui/ppo.sh
    gui/plv.sh
    gui/pin.sh
)

# ── Quick connectivity check ───────────────────────────────────────────────────
printf '\n%b%b Zappy — Defense Test Suite%b\n' "$BOLD" "$YELLOW" "$NC"
printf '%b  Server: %s:%s%b\n\n' "$DIM" "$ZAPPY_HOST" "$ZAPPY_PORT" "$NC"

if ! bash -c "exec 3<>/dev/tcp/$ZAPPY_HOST/$ZAPPY_PORT" 2>/dev/null; then
    printf '%bServer not reachable at %s:%s%b\n' "$RED" "$ZAPPY_HOST" "$ZAPPY_PORT" "$NC"
    printf 'Start it with:\n'
    printf '  ./zappy_server -p %s -x 10 -y 10 -n team1 team2 -c 5 -f 10\n\n' "$ZAPPY_PORT"
    exit 1
fi

# ── Runner ─────────────────────────────────────────────────────────────────────
run_script() {
    local script="$1"
    local path="$SCRIPT_DIR/$script"
    if [[ ! -f "$path" ]]; then
        printf '  %bSKIP%b  %s (file not found)\n' "$YELLOW" "$NC" "$script"
        return
    fi
    chmod +x "$path"
    # Capture output; extract pass/fail counts from the summary line
    OUTPUT=$(bash "$path" 2>&1)
    EXIT_CODE=$?
    echo "$OUTPUT"
    if [[ $EXIT_CODE -eq 0 ]]; then
        TOTAL_PASS=$((TOTAL_PASS + 1))
    else
        TOTAL_FAIL=$((TOTAL_FAIL + 1))
        FAILED_SCRIPTS+=("$script")
    fi
    echo ""
}

printf '%b%b── AI Commands (%d scripts) ──────────────────────────────────────────%b\n' \
    "$BOLD" "\033[36m" "${#AI_SCRIPTS[@]}" "$NC"
for s in "${AI_SCRIPTS[@]}"; do run_script "$s"; done

printf '%b%b── GUI Commands (%d scripts) ─────────────────────────────────────────%b\n' \
    "$BOLD" "\033[36m" "${#GUI_SCRIPTS[@]}" "$NC"
for s in "${GUI_SCRIPTS[@]}"; do run_script "$s"; done

# ── Final summary ──────────────────────────────────────────────────────────────
TOTAL=$((TOTAL_PASS + TOTAL_FAIL))
printf '\n%b%b══════════════════════════════════════════════════════════════%b\n' "$BOLD" "\033[36m" "$NC"
printf '%b  Defense Suite Results%b\n' "$BOLD" "$NC"
printf '%b══════════════════════════════════════════════════════════════%b\n\n' "$BOLD" "$NC"

if [[ $TOTAL_FAIL -eq 0 ]]; then
    printf '  %b%bAll %d test scripts passed%b\n' "$GREEN" "$BOLD" "$TOTAL" "$NC"
else
    printf '  %b%b%d/%d scripts passed  (%d failed)%b\n' \
        "$RED" "$BOLD" "$TOTAL_PASS" "$TOTAL" "$TOTAL_FAIL" "$NC"
    printf '\n  %bFailed scripts:%b\n' "$RED" "$NC"
    for s in "${FAILED_SCRIPTS[@]}"; do
        printf '    • %s\n' "$s"
    done
fi
printf '\n%b══════════════════════════════════════════════════════════════%b\n\n' "$BOLD" "$NC"

[[ $TOTAL_FAIL -eq 0 ]]
