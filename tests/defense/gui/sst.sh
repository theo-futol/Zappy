#!/usr/bin/env bash
# GUI command: sst T
# Sets the server frequency to T.
# Server response: "sst T"
# This test changes the frequency to 50, verifies the response, then restores it to 10.
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "GUI — sst T"

zappy_gui_connect

# Change frequency to 50
zappy_send "sst 50"
sleep 0.10
resp=$(zappy_read)
note "sst 50 → $resp"
check_eq "sst — set frequency to 50" "$resp" "sst 50"

# Restore to 10
zappy_send "sst 10"
sleep 0.10
restore=$(zappy_read)
note "sst 10 → $restore"
check_eq "sst — restore frequency to 10" "$restore" "sst 10"

zappy_exit
