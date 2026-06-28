#!/usr/bin/env bash
###############################################################################
## EPITECH PROJECT - zappy                                                   ##
## tests/stress_server.sh                                                    ##
##   Stress / fuzz test suite for the Zappy server.                          ##
##   The goal is to TRY TO BREAK an already-running server: garbage input,   ##
##   /dev/null & /dev/urandom floods, partial packets, giant lines,          ##
##   connection floods, slot overflow, abrupt disconnects, etc.              ##
##                                                                           ##
##   Usage: ./stress_server.sh [host] [port] [team]                          ##
##          HOST/PORT/TEAM can also be set via env vars.                     ##
##                                                                           ##
##   The server is assumed to be ALREADY RUNNING. After every attack the     ##
##   suite re-checks that the server still accepts connections and still     ##
##   sends the WELCOME handshake. If it ever stops, the server is declared   ##
##   DOWN and the script exits non-zero.                                     ##
###############################################################################

set -u

# --------------------------------------------------------------------------- #
# Configuration
# --------------------------------------------------------------------------- #
HOST="${1:-${ZAPPY_HOST:-127.0.0.1}}"
PORT="${2:-${ZAPPY_PORT:-4242}}"
TEAM="${3:-${ZAPPY_TEAM:-team1}}"

# Tunables (override via env)
FLOOD_CONNECTIONS="${FLOOD_CONNECTIONS:-300}"   # simultaneous connections
RECONNECT_STORM="${RECONNECT_STORM:-500}"       # rapid connect/disconnect cycles
GIANT_LINE_MB="${GIANT_LINE_MB:-8}"             # size of the no-newline blast
COMMAND_FLOOD="${COMMAND_FLOOD:-5000}"          # commands spammed on one socket
NET_TIMEOUT="${NET_TIMEOUT:-3}"                 # seconds for a health probe
GUI_CLIENTS="${GUI_CLIENTS:-1000}"              # GUI threads for mass/thread-flood tests
GUI_CMD_REPEAT="${GUI_CMD_REPEAT:-50}"          # command-block repeats per GUI thread

# --------------------------------------------------------------------------- #
# Pretty output
# --------------------------------------------------------------------------- #
if [ -t 1 ]; then
    C_RED=$'\033[31m'; C_GRN=$'\033[32m'; C_YEL=$'\033[33m'
    C_BLU=$'\033[34m'; C_BLD=$'\033[1m';  C_RST=$'\033[0m'
else
    C_RED=""; C_GRN=""; C_YEL=""; C_BLU=""; C_BLD=""; C_RST=""
fi

PASS=0
FAIL=0
TOTAL=0

log()   { printf '%s\n' "$*"; }
title() { printf '\n%s== %s ==%s\n' "$C_BLU$C_BLD" "$*" "$C_RST"; }

# --------------------------------------------------------------------------- #
# Dependency check
# --------------------------------------------------------------------------- #
PY=""
for c in python3 python; do
    if command -v "$c" >/dev/null 2>&1; then PY="$c"; break; fi
done
if [ -z "$PY" ]; then
    log "${C_RED}python3 is required for this suite.${C_RST}"
    exit 2
fi

# --------------------------------------------------------------------------- #
# Health probe: returns 0 if the server still answers WELCOME, 1 otherwise.
# This is our crash detector.
# --------------------------------------------------------------------------- #
health_check() {
    "$PY" - "$HOST" "$PORT" "$NET_TIMEOUT" <<'PYEOF'
import socket, sys
host, port, to = sys.argv[1], int(sys.argv[2]), float(sys.argv[3])
try:
    s = socket.create_connection((host, port), timeout=to)
    s.settimeout(to)
    data = s.recv(64)
    s.close()
    sys.exit(0 if data.startswith(b"WELCOME") else 1)
except Exception:
    sys.exit(1)
PYEOF
}

# Run a named attack (its body is the rest of the args), then verify the
# server survived. Records PASS / FAIL.
run_attack() {
    local name="$1"; shift
    TOTAL=$((TOTAL + 1))
    printf '%s[%02d]%s %-45s ' "$C_BLD" "$TOTAL" "$C_RST" "$name"

    # Execute the attack body; never let its failure abort the suite.
    ( "$@" ) >/dev/null 2>&1

    if health_check; then
        printf '%sSURVIVED%s\n' "$C_GRN" "$C_RST"
        PASS=$((PASS + 1))
        return 0
    else
        printf '%sSERVER DOWN%s\n' "$C_RED$C_BLD" "$C_RST"
        FAIL=$((FAIL + 1))
        return 1
    fi
}

# --------------------------------------------------------------------------- #
# Generic raw-socket sender (python). Reads payload from stdin, sends it,
# optionally lingers, then closes. Args: host port [linger_seconds]
# --------------------------------------------------------------------------- #
raw_send() {
    "$PY" - "$HOST" "$PORT" "${1:-0}" <<'PYEOF'
import socket, sys
host, port, linger = sys.argv[1], int(sys.argv[2]), float(sys.argv[3])
payload = sys.stdin.buffer.read()
try:
    s = socket.create_connection((host, port), timeout=3)
    s.settimeout(3)
    try: s.recv(64)          # consume WELCOME
    except Exception: pass
    if payload:
        s.sendall(payload)
    if linger > 0:
        import time; time.sleep(linger)
    s.close()
except Exception:
    pass
PYEOF
}

# =========================================================================== #
# ATTACKS
# =========================================================================== #

# --- 1. Empty input / /dev/null ------------------------------------------- #
atk_devnull() {
    cat /dev/null | raw_send 0
}

# --- 2. Random binary blast from /dev/urandom ----------------------------- #
atk_urandom() {
    head -c 65536 /dev/urandom | raw_send 0
}

# --- 3. Null bytes & control chars ---------------------------------------- #
atk_nullbytes() {
    "$PY" - "$HOST" "$PORT" <<'PYEOF'
import socket, sys
host, port = sys.argv[1], int(sys.argv[2])
payload = b"\x00\x00\x00GRAPHIC\x00\n\x01\x02\x03\xff\xfe\nmsz\x00\n"
try:
    s = socket.create_connection((host, port), timeout=3); s.settimeout(3)
    try: s.recv(64)
    except Exception: pass
    s.sendall(payload); s.close()
except Exception: pass
PYEOF
}

# --- 4. Garbage handshake / unknown commands ------------------------------ #
atk_bad_handshake() {
    printf 'NOT_A_TEAM\nlol\n????\n%s\n' "$(head -c 2000 /dev/urandom | tr -dc 'a-zA-Z0-9' | head -c 1000)" | raw_send 0
}

# --- 5. Unknown commands as AI & GUI -------------------------------------- #
atk_unknown_cmds() {
    printf '%s\nForward\nfoobar\n42\nGRAPHIC msz\n;rm -rf /\n$(whoami)\n`id`\n' "$TEAM" | raw_send 0
    printf 'GRAPHIC\nzzz\nmsz extra args here\nbct -1 -1\nppo 999999\n' | raw_send 0
}

# --- 6. Partial packet, no trailing newline, hold the socket open --------- #
atk_partial_noeol() {
    printf 'Forward without a newline ever' | raw_send 1
}

# --- 7. Byte-by-byte slow trickle (slowloris-style) ----------------------- #
atk_slowloris() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, sys, time
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
try:
    s = socket.create_connection((host, port), timeout=3); s.settimeout(3)
    try: s.recv(64)
    except Exception: pass
    for ch in (team + "\nLook\n"):
        try: s.sendall(ch.encode()); time.sleep(0.01)
        except Exception: break
    s.close()
except Exception: pass
PYEOF
}

# --- 8. Giant line with no newline (unbounded buffer growth) -------------- #
atk_giant_line() {
    "$PY" - "$HOST" "$PORT" "$GIANT_LINE_MB" <<'PYEOF'
import socket, sys
host, port, mb = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
chunk = b"A" * (1024 * 1024)
try:
    s = socket.create_connection((host, port), timeout=5); s.settimeout(5)
    try: s.recv(64)
    except Exception: pass
    for _ in range(mb):
        s.sendall(chunk)          # never a '\n' -> server keeps buffering
    s.close()
except Exception: pass
PYEOF
}

# --- 9. Command flood on a single socket (hits the ban system) ------------ #
atk_command_flood() {
    "$PY" - "$HOST" "$PORT" "$TEAM" "$COMMAND_FLOOD" <<'PYEOF'
import socket, sys
host, port, team, n = sys.argv[1], int(sys.argv[2]), sys.argv[3], int(sys.argv[4])
try:
    s = socket.create_connection((host, port), timeout=5); s.settimeout(5)
    try: s.recv(64)
    except Exception: pass
    s.sendall((team + "\n").encode())
    buf = b"Forward\nLook\nInventory\nRight\nLeft\n" * 50
    for _ in range(max(1, n // 250)):
        try: s.sendall(buf)
        except Exception: break
    s.close()
except Exception: pass
PYEOF
}

# --- 10. Connection flood (many simultaneous sockets) --------------------- #
atk_connection_flood() {
    "$PY" - "$HOST" "$PORT" "$FLOOD_CONNECTIONS" "$TEAM" <<'PYEOF'
import socket, sys
host, port, n, team = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
socks = []
for _ in range(n):
    try:
        s = socket.create_connection((host, port), timeout=3)
        s.setblocking(False)
        try: s.send((team + "\n").encode())
        except Exception: pass
        socks.append(s)
    except Exception:
        pass
for s in socks:
    try: s.close()
    except Exception: pass
PYEOF
}

# --- 11. Slot overflow: more AI clients than -c allows -------------------- #
atk_slot_overflow() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, sys
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
socks = []
for _ in range(200):                 # far more than any reasonable -c
    try:
        s = socket.create_connection((host, port), timeout=3); s.settimeout(2)
        try: s.recv(64)
        except Exception: pass
        s.sendall((team + "\n").encode())
        socks.append(s)
    except Exception:
        pass
for s in socks:
    try: s.close()
    except Exception: pass
PYEOF
}

# --- 12. Abrupt disconnect (RST) mid-command ------------------------------ #
atk_abrupt_rst() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, struct, sys
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
for _ in range(100):
    try:
        s = socket.create_connection((host, port), timeout=2)
        # SO_LINGER with timeout 0 -> close() sends a RST instead of FIN
        s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
        try: s.send((team + "\nForwa").encode())
        except Exception: pass
        s.close()
    except Exception:
        pass
PYEOF
}

# --- 13. Reconnect storm (rapid connect/handshake/close) ------------------ #
atk_reconnect_storm() {
    "$PY" - "$HOST" "$PORT" "$TEAM" "$RECONNECT_STORM" <<'PYEOF'
import socket, sys
host, port, team, n = sys.argv[1], int(sys.argv[2]), sys.argv[3], int(sys.argv[4])
for _ in range(n):
    try:
        s = socket.create_connection((host, port), timeout=2)
        try:
            s.settimeout(0.2); s.recv(64)
            s.sendall((team + "\n").encode())
        except Exception:
            pass
        s.close()
    except Exception:
        pass
PYEOF
}

# --- 14. Oversized single token / huge numbers ---------------------------- #
atk_huge_tokens() {
    local big
    big=$(head -c 100000 /dev/zero | tr '\0' 'B')
    printf '%s\nBroadcast %s\nSet 999999999999999999999\nTake -2147483648\n' "$TEAM" "$big" | raw_send 0
}

# --- 15. Malformed GUI commands & negative coordinates -------------------- #
atk_gui_malformed() {
    printf 'GRAPHIC\nbct\nbct abc def\nbct -5 -5\nplv 0\nplv -1\npin 99999999\nsgt -1\nsst 0\nmsz a b c d\n' | raw_send 0
}

# --- 16. /dev/urandom piped continuously while flooding ------------------- #
atk_urandom_stream() {
    "$PY" - "$HOST" "$PORT" <<'PYEOF'
import socket, os, sys
host, port = sys.argv[1], int(sys.argv[2])
try:
    s = socket.create_connection((host, port), timeout=3); s.settimeout(3)
    try: s.recv(64)
    except Exception: pass
    for _ in range(64):
        try: s.sendall(os.urandom(4096))   # crosses recv() boundaries
        except Exception: break
    s.close()
except Exception: pass
PYEOF
}

# =========================================================================== #
# HARDER ATTACKS — concurrency, backpressure, resource exhaustion
# =========================================================================== #

# --- 17. CRLF / tabs / whitespace / empty-line flood ---------------------- #
atk_crlf_whitespace() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, sys
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
payload = (team + "\r\n").encode()
payload += b"\n\n\n\n\n"                       # empty-line flood
payload += b"   Forward   \r\n\tLook\t\r\n"    # leading/trailing ws + tabs
payload += b" \t \r\n" * 200
try:
    s = socket.create_connection((host, port), timeout=3); s.settimeout(3)
    try: s.recv(64)
    except Exception: pass
    s.sendall(payload); s.close()
except Exception: pass
PYEOF
}

# --- 18. Mixed valid + binary garbage on the same socket ------------------ #
atk_mixed_valid_garbage() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, os, sys
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
try:
    s = socket.create_connection((host, port), timeout=4); s.settimeout(4)
    try: s.recv(64)
    except Exception: pass
    s.sendall((team + "\n").encode())
    for _ in range(50):
        s.sendall(b"Look\n" + os.urandom(200) + b"\nInventory\n" + os.urandom(50))
    s.close()
except Exception: pass
PYEOF
}

# --- 19. Incantation abuse (frozen/begin edge cases) ---------------------- #
atk_incantation_abuse() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, sys
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
try:
    s = socket.create_connection((host, port), timeout=4); s.settimeout(4)
    try: s.recv(64)
    except Exception: pass
    s.sendall((team + "\n").encode())
    s.sendall(b"Incantation\n" * 30)   # spam ritual begin while frozen
    s.close()
except Exception: pass
PYEOF
}

# --- 20. Slot churn: hammer connect/handshake/disconnect on a team -------- #
#         Stresses slot accounting (getAvailableSlotsForTeam underflow).
atk_slot_churn() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, sys
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
for _ in range(400):
    try:
        s = socket.create_connection((host, port), timeout=2); s.settimeout(1)
        try: s.recv(64)
        except Exception: pass
        s.sendall((team + "\n").encode())
        try: s.recv(64)
        except Exception: pass
        s.close()                       # free the slot immediately
    except Exception:
        pass
PYEOF
}

# --- 21. Concurrent mixed load (REAL parallelism, several seconds) -------- #
atk_concurrent_load() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, os, sys, threading, time
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
stop = time.time() + 4
def worker():
    while time.time() < stop:
        try:
            s = socket.create_connection((host, port), timeout=2); s.settimeout(2)
            try: s.recv(64)
            except Exception: pass
            s.sendall((team + "\n").encode())
            s.sendall(b"Forward\nLook\n" + os.urandom(100) + b"\nInventory\n")
            s.close()
        except Exception:
            pass
ts = [threading.Thread(target=worker) for _ in range(40)]
for t in ts: t.start()
for t in ts: t.join()
PYEOF
}

# --- 22. File-descriptor exhaustion --------------------------------------- #
#         Open as many sockets as the OS lets us, hold them, then release.
atk_fd_exhaustion() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, sys
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
socks = []
try:
    for _ in range(2000):
        try:
            s = socket.create_connection((host, port), timeout=2)
            s.setblocking(False)
            try: s.send((team + "\n").encode())
            except Exception: pass
            socks.append(s)
        except Exception:
            break                       # ran out of fds (ours or theirs)
finally:
    for s in socks:
        try: s.close()
        except Exception: pass
PYEOF
}

# --- 23. Dead reader + broadcast backpressure (blocking-send DoS) --------- #
#   Many GUI clients that NEVER read, plus AI clients spamming Broadcast.
#   If the server uses blocking send(), full kernel buffers freeze it.
#   Sockets are released at the end so a healthy server recovers.
atk_dead_reader_backpressure() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, sys, time
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
guis, ais = [], []
try:
    # GUI clients that never read -> their recv buffers fill up.
    for _ in range(30):
        try:
            g = socket.create_connection((host, port), timeout=2)
            g.sendall(b"GRAPHIC\n")
            guis.append(g)
        except Exception:
            pass
    # AI clients spamming Broadcast -> server tries to push to every GUI.
    for _ in range(5):
        try:
            a = socket.create_connection((host, port), timeout=2); a.settimeout(2)
            try: a.recv(64)
            except Exception: pass
            a.sendall((team + "\n").encode())
            a.sendall(b"Broadcast flooooooooooooooooood\n" * 100)
            ais.append(a)
        except Exception:
            pass
    time.sleep(1)
finally:
    for s in guis + ais:
        try: s.close()
        except Exception: pass
PYEOF
}

# --- 24. AI command queue overflow: send far more commands than the server   #
#         queues (protocol cap is 10). Never reads responses, so the queue    #
#         stays full and the server must drop or reject the excess without    #
#         crashing. Three clients run in parallel to amplify the pressure.   #
atk_ai_queue_overflow() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, sys, threading
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
# 200 commands >> any sane server queue limit (protocol mandates max 10).
flood = (b"Forward\nLeft\nRight\nLook\nInventory\n" * 40)   # 200 commands

def spammer():
    try:
        s = socket.create_connection((host, port), timeout=3); s.settimeout(3)
        try: s.recv(64)              # consume WELCOME
        except Exception: pass
        s.sendall((team + "\n").encode())
        try: s.recv(64)              # consume slot count
        except Exception: pass
        # Dump the whole flood without reading a single response.
        try: s.sendall(flood)
        except Exception: pass
        # Linger briefly so the server has time to try processing.
        import time; time.sleep(0.5)
        s.close()
    except Exception:
        pass

ts = [threading.Thread(target=spammer) for _ in range(3)]
for t in ts: t.start()
for t in ts: t.join()
PYEOF
}

# --- 25. AI queue drain loop: send a command, read the response, repeat     #
#         in a tight loop. Stresses the full request/response cycle and       #
#         ensures the server doesn't stall or corrupt state across many       #
#         sequential transactions on the same connection.                     #
atk_ai_queue_drain_loop() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, sys, time
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
cmds = [b"Look\n", b"Inventory\n", b"Forward\n", b"Left\n", b"Right\n"]
deadline = time.time() + 3
try:
    s = socket.create_connection((host, port), timeout=3); s.settimeout(1)
    try: s.recv(64)
    except Exception: pass
    s.sendall((team + "\n").encode())
    try: s.recv(64)
    except Exception: pass
    i = 0
    while time.time() < deadline:
        try:
            s.sendall(cmds[i % len(cmds)])
            i += 1
            try: s.recv(256)         # drain one response before sending next
            except Exception: pass
        except Exception:
            break
    s.close()
except Exception:
    pass
PYEOF
}

# --- 27. GUI command queue overflow: connect as GRAPHIC and dump hundreds of  #
#         commands without ever reading a response. The server must not crash  #
#         or deadlock when the GUI-side output buffer backs up.               #
atk_gui_queue_overflow() {
    "$PY" - "$HOST" "$PORT" <<'PYEOF'
import socket, sys, threading
host, port = sys.argv[1], int(sys.argv[2])
# Mix of all GUI commands; none require arguments except bct/ppo/plv/pin.
flood = (
    b"msz\nmct\ntna\nsgt\n"
    b"bct 0 0\nbct 1 1\nbct 2 2\n"
    b"ppo 1\nplv 1\npin 1\n"
) * 30   # ~270 commands

def gui_spammer():
    try:
        s = socket.create_connection((host, port), timeout=3); s.settimeout(3)
        try: s.recv(64)              # consume WELCOME
        except Exception: pass
        s.sendall(b"GRAPHIC\n")
        # Dump the whole flood without reading a single response.
        try: s.sendall(flood)
        except Exception: pass
        import time; time.sleep(0.5)
        s.close()
    except Exception:
        pass

ts = [threading.Thread(target=gui_spammer) for _ in range(3)]
for t in ts: t.start()
for t in ts: t.join()
PYEOF
}

# --- 28. GUI drain loop: send one GUI command, read the response, repeat      #
#         in a tight loop. Stresses the full GUI request/response cycle and    #
#         ensures map-state serialisation stays consistent under rapid queries. #
atk_gui_drain_loop() {
    "$PY" - "$HOST" "$PORT" <<'PYEOF'
import socket, sys, time
host, port = sys.argv[1], int(sys.argv[2])
cmds = [b"msz\n", b"mct\n", b"tna\n", b"sgt\n", b"bct 0 0\n", b"ppo 1\n"]
deadline = time.time() + 3
try:
    s = socket.create_connection((host, port), timeout=3); s.settimeout(1)
    try: s.recv(64)
    except Exception: pass
    s.sendall(b"GRAPHIC\n")
    i = 0
    while time.time() < deadline:
        try:
            s.sendall(cmds[i % len(cmds)])
            i += 1
            try: s.recv(512)         # drain one response before next command
            except Exception: pass
        except Exception:
            break
    s.close()
except Exception:
    pass
PYEOF
}

# --- 29. GUI mass connection: open 1 000 GRAPHIC sockets simultaneously,     #
#         send a burst of commands on each, hold them, then release.          #
#         Tests the server's ability to manage a huge GUI subscriber list     #
#         and broadcast map events to every one of them without freezing.     #
atk_gui_mass_connect() {
    "$PY" - "$HOST" "$PORT" "$GUI_CLIENTS" <<'PYEOF'
import socket, sys, threading, time
host, port, TARGET = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
burst  = b"msz\nmct\ntna\nsgt\n"
lock   = threading.Lock()
socks  = []

def connect_one():
    try:
        s = socket.create_connection((host, port), timeout=3); s.settimeout(2)
        try: s.recv(64)
        except Exception: pass
        s.sendall(b"GRAPHIC\n")
        try: s.sendall(burst)
        except Exception: pass
        with lock:
            socks.append(s)
    except Exception:
        pass

ts = [threading.Thread(target=connect_one) for _ in range(TARGET)]
for t in ts: t.start()
for t in ts: t.join()

time.sleep(1)           # hold all sockets open briefly

for s in socks:
    try: s.close()
    except Exception: pass
PYEOF
}

# --- 30. GUI thread flood: 1 000 threads each open their own GRAPHIC socket  #
#         and independently spam hundreds of GUI commands without reading any  #
#         response. Combines mass-connection pressure with per-socket output  #
#         buffer saturation across the entire subscriber list at once.        #
atk_gui_thread_flood() {
    "$PY" - "$HOST" "$PORT" "$GUI_CLIENTS" "$GUI_CMD_REPEAT" <<'PYEOF'
import socket, sys, threading, time
host, port, TARGET, repeat = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
flood   = (
    b"msz\nmct\ntna\nsgt\n"
    b"bct 0 0\nbct 1 1\nbct 2 2\n"
    b"ppo 1\nplv 1\npin 1\n"
) * repeat

lock  = threading.Lock()
socks = []

def gui_flood_worker():
    try:
        s = socket.create_connection((host, port), timeout=3); s.settimeout(3)
        try: s.recv(64)              # consume WELCOME
        except Exception: pass
        s.sendall(b"GRAPHIC\n")
        try: s.sendall(flood)        # dump all commands, never read back
        except Exception: pass
        with lock:
            socks.append(s)
    except Exception:
        pass

ts = [threading.Thread(target=gui_flood_worker) for _ in range(TARGET)]
for t in ts: t.start()
for t in ts: t.join()

time.sleep(1)   # hold sockets open so the server must juggle all of them

for s in socks:
    try: s.close()
    except Exception: pass
PYEOF
}

# --- 32. Half-close (shutdown SHUT_WR) to exercise POLLHUP path ------------ #
atk_half_close() {
    "$PY" - "$HOST" "$PORT" "$TEAM" <<'PYEOF'
import socket, sys
host, port, team = sys.argv[1], int(sys.argv[2]), sys.argv[3]
for _ in range(100):
    try:
        s = socket.create_connection((host, port), timeout=2)
        try: s.recv(64)
        except Exception: pass
        s.sendall((team + "\nForward\n").encode())
        s.shutdown(socket.SHUT_WR)      # write-half closed, read-half open
        try: s.recv(64)
        except Exception: pass
        s.close()
    except Exception:
        pass
PYEOF
}

# =========================================================================== #
# MAIN
# =========================================================================== #
main() {
    printf '%s%sZappy server stress test%s\n' "$C_BLD" "$C_YEL" "$C_RST"
    log "Target : $HOST:$PORT   Team: $TEAM"
    log "Flood  : $FLOOD_CONNECTIONS conns, $RECONNECT_STORM reconnects, ${GIANT_LINE_MB}MB blast"
    log "GUI    : $GUI_CLIENTS clients, ${GUI_CMD_REPEAT}x cmd-block per thread"

    title "Pre-flight"
    if ! health_check; then
        log "${C_RED}${C_BLD}Server is not reachable on $HOST:$PORT (no WELCOME).${C_RST}"
        log "Start it first, e.g.: ./zappy_server -p $PORT -x 10 -y 10 -n $TEAM -c 5"
        exit 2
    fi
    log "${C_GRN}Server reachable, WELCOME received. Starting attacks...${C_RST}"

    title "Attacks"
    run_attack "/dev/null (empty payload)"           atk_devnull
    run_attack "Random binary blast (/dev/urandom)"  atk_urandom
    run_attack "NUL bytes & control chars"           atk_nullbytes
    run_attack "Garbage handshake"                   atk_bad_handshake
    run_attack "Unknown AI/GUI commands"             atk_unknown_cmds
    run_attack "Partial packet, no newline"          atk_partial_noeol
    run_attack "Slowloris byte trickle"              atk_slowloris
    run_attack "Giant line, no newline (mem growth)" atk_giant_line
    run_attack "Command flood (ban system)"          atk_command_flood
    run_attack "Connection flood"                    atk_connection_flood
    run_attack "Slot overflow"                       atk_slot_overflow
    run_attack "Abrupt RST mid-command"              atk_abrupt_rst
    run_attack "Reconnect storm"                     atk_reconnect_storm
    run_attack "Oversized tokens & huge numbers"     atk_huge_tokens
    run_attack "Malformed GUI commands"              atk_gui_malformed
    run_attack "/dev/urandom streaming"              atk_urandom_stream

    title "Harder attacks (concurrency / backpressure / exhaustion)"
    run_attack "CRLF / tabs / empty-line flood"      atk_crlf_whitespace
    run_attack "Mixed valid + binary garbage"        atk_mixed_valid_garbage
    run_attack "Incantation abuse"                   atk_incantation_abuse
    run_attack "Slot churn (accounting underflow)"   atk_slot_churn
    run_attack "Concurrent mixed load (40 threads)"  atk_concurrent_load
    run_attack "File-descriptor exhaustion"          atk_fd_exhaustion
    run_attack "Dead reader + broadcast backpressure" atk_dead_reader_backpressure
    run_attack "Half-close (POLLHUP path)"           atk_half_close
    run_attack "AI queue overflow (200 cmds, no read)" atk_ai_queue_overflow
    run_attack "AI queue drain loop (send/recv cycle)" atk_ai_queue_drain_loop
    run_attack "GUI queue overflow (270 cmds, no read)" atk_gui_queue_overflow
    run_attack "GUI drain loop (send/recv cycle)"       atk_gui_drain_loop
    run_attack "GUI mass connect (1000 simultaneous)"   atk_gui_mass_connect
    run_attack "GUI thread flood (1000 threads x 500 cmds)" atk_gui_thread_flood

    title "Summary"
    log "Total attacks : $TOTAL"
    log "${C_GRN}Survived      : $PASS${C_RST}"
    if [ "$FAIL" -gt 0 ]; then
        log "${C_RED}${C_BLD}Server down   : $FAIL${C_RST}"
        log "${C_RED}${C_BLD}RESULT: FAIL — the server crashed or stopped responding.${C_RST}"
        exit 1
    fi
    log "${C_GRN}${C_BLD}RESULT: PASS — the server survived every attack.${C_RST}"
    exit 0
}

main "$@"
