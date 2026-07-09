#!/usr/bin/env bash
# GUI command: sgt
# Requests the current server frequency (time unit).
# Server response: "sgt T"  where T is the current f value.
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "GUI — sgt"

zappy_gui_connect
zappy_send "sgt"
sleep 0.10
resp=$(zappy_read)

note "response: $resp"
check_regex "sgt — get server frequency" "$resp" '^sgt [0-9]+$'
zappy_exit
