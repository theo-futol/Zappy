#!/usr/bin/env bash
# GUI command: mct
# Requests the resource content of the entire map (one bct line per tile).
# Server response: one "bct X Y q0 … q6" line per tile (100 lines for a 10×10 map).
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "GUI — mct"

zappy_gui_connect
zappy_send "mct"

# Read all bct lines the server sends back
MCT_LINES=()
while IFS= read -r -t 1.0 ln <&3; do
    ln="${ln%$'\r'}"
    [[ "$ln" == bct* ]] && MCT_LINES+=("$ln")
done

note "received ${#MCT_LINES[@]} bct lines"
if (( ${#MCT_LINES[@]} > 0 )); then
    note "first: ${MCT_LINES[0]}"
    note "last:  ${MCT_LINES[-1]}"
fi

check_regex "mct — full map (≥ 10 bct lines)" "${#MCT_LINES[@]}" '^[1-9][0-9]*$'

# Verify every collected line starts with "bct"
ALL_BCT=true
for ln in "${MCT_LINES[@]}"; do
    [[ "$ln" == bct* ]] || { ALL_BCT=false; break; }
done
$ALL_BCT && _pass "mct — all lines are valid bct responses" "${#MCT_LINES[@]} lines" \
          || _fail "mct — some lines are not bct" ""

zappy_exit
