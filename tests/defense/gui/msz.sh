#!/usr/bin/env bash
# GUI command: msz
# Requests the map dimensions.
# Server response: "msz X Y"
set -euo pipefail
source "$(dirname "$0")/../lib.sh"

banner "GUI — msz"

zappy_gui_connect
zappy_send "msz"
sleep 0.10
resp=$(zappy_read)

note "response: $resp"
check_regex "msz — map dimensions" "$resp" '^msz [0-9]+ [0-9]+$'
zappy_exit
