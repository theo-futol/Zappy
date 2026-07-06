#!/usr/bin/env bash
# GUI command: tna
# Requests the name of every team.
# Server response: one "tna <name>" line per team.
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "GUI — tna"

zappy_gui_connect
zappy_send "tna"

# Read team-name lines (drain for up to 0.5 s)
TNA_LINES=()
while IFS= read -r -t 0.5 ln <&3; do
    ln="${ln%$'\r'}"
    [[ "$ln" == tna* ]] && TNA_LINES+=("$ln")
done

note "received ${#TNA_LINES[@]} team(s)"
for ln in "${TNA_LINES[@]}"; do note "  $ln"; done

check_regex "tna — at least one team name" "${#TNA_LINES[@]}" '^[1-9][0-9]*$'
check_prefix "tna — first response format" "${TNA_LINES[0]:-}" "tna "
zappy_exit
