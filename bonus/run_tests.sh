#!/usr/bin/env bash
#
# Run every test suite of the bonus/ services and print a categorized summary.
#
# Categories:
#   1. ZapLauncher functional  — pytest vs a fake Zappy server + scripted LLM (no game server needed)
#   2. GUI bridge              — node --test driving a real bridge process vs a fake TCP server
#   3. Real-server e2e         — pytest vs the compose-built C++ server (production wait-mode config)
#   4. Playbook scene          — the village-gathering conversation scene vs the real server (classic mode)
#
# Everything runs inside the Docker images the project already uses
# (python:trixie, node:22-alpine, the compose `server` image).
#
# Usage:
#   ./run_tests.sh          # all categories
#   ./run_tests.sh --fast   # only categories 1 & 2 (no server build/boot)
#
set -u
cd "$(dirname "$0")"

DOCKER="docker"
$DOCKER info >/dev/null 2>&1 || DOCKER="docker --context default"
$DOCKER info >/dev/null 2>&1 || { echo "Error: no reachable docker daemon"; exit 1; }

FAST=0
[ "${1:-}" = "--fast" ] && FAST=1

PIP_CACHE="zappy-test-pip"   # named volume: avoids re-downloading wheels every run

GREEN=$'\e[32m'; RED=$'\e[31m'; YELLOW=$'\e[33m'; BOLD=$'\e[1m'; RESET=$'\e[0m'

declare -a CAT_NAME CAT_RESULT CAT_SUMMARY

record() {  # name, exit_code, summary
    CAT_NAME+=("$1")
    if [ "$2" -eq 0 ]; then CAT_RESULT+=("${GREEN}PASS${RESET}"); else CAT_RESULT+=("${RED}FAIL${RESET}"); fi
    CAT_SUMMARY+=("$3")
}

banner() { echo; echo "${BOLD}=== $1 ===${RESET}"; }

server_up() {  # extra env assignments as arguments
    env "$@" $DOCKER compose up -d --force-recreate server >/dev/null 2>&1
    sleep 2
}

cleanup() { $DOCKER compose stop server >/dev/null 2>&1 || true; }
trap cleanup EXIT

# --- 1. ZapLauncher functional ------------------------------------------------

banner "1/4  ZapLauncher — functional & API tests (python:trixie, fake server)"
out=$($DOCKER run --rm -v "$PWD/ZapLauncher:/app" -w /app \
      -v "$PIP_CACHE:/root/.cache/pip" -e API_KEY=test python:trixie \
      bash -c "pip install -q -r requirements-dev.txt && python -m pytest -p no:warnings" 2>&1)
code=$?
echo "$out" | tail -3
record "ZapLauncher functional" $code \
       "$(echo "$out" | grep -E '[0-9]+ (passed|failed)' | tail -1 | xargs)"

# --- 2. GUI bridge --------------------------------------------------------------

banner "2/4  GUI bridge — GameState/protocol tests (node:22-alpine, fake server)"
out=$($DOCKER run --rm -v "$PWD/GUI:/gui" -w /gui/bridge node:22-alpine \
      sh -c "npm install --silent >/dev/null 2>&1; npm test" 2>&1)
code=$?
echo "$out" | grep -E "^# (tests|pass|fail)" || echo "$out" | tail -5
record "GUI bridge" $code \
       "$(echo "$out" | grep -E '^# (pass|fail)' | tr -d '#' | xargs)"

if [ "$FAST" -eq 1 ]; then
    echo
    echo "${YELLOW}--fast: skipping real-server categories (3 & 4)${RESET}"
    record "Real-server e2e" 0 "skipped (--fast)"
    CAT_RESULT[2]="${YELLOW}SKIP${RESET}"
    record "Playbook scene" 0 "skipped (--fast)"
    CAT_RESULT[3]="${YELLOW}SKIP${RESET}"
else
    # --- 3. Real-server e2e (production config: wait mode) ---------------------

    banner "3/4  Real-server e2e — C++ server in production wait-mode (compose)"
    $DOCKER compose build -q server >/dev/null 2>&1 || $DOCKER compose build server
    server_up ZAPPY_CLIENTS=20
    out=$($DOCKER run --rm --network bonus_zappy-net -v "$PWD/ZapLauncher:/app" -w /app \
          -v "$PIP_CACHE:/root/.cache/pip" \
          -e API_KEY=test -e ZAPPY_E2E_HOST=server -e ZAPPY_E2E_PORT=4242 python:trixie \
          bash -c "pip install -q -r requirements-dev.txt && python -m pytest tests/test_e2e_real_server.py -p no:warnings" 2>&1)
    code=$?
    echo "$out" | tail -3
    record "Real-server e2e" $code \
           "$(echo "$out" | grep -E '[0-9]+ (passed|failed)' | tail -1 | xargs)"

    # --- 4. Playbook scene (classic mode: fast serial-free turns) --------------

    banner "4/4  Playbook scene — village-gathering conversation (classic mode)"
    server_up ZAPPY_CLIENTS=20 ZAPPY_WAIT_TIMEOUT=
    out=$($DOCKER run --rm --network bonus_zappy-net -v "$PWD/ZapLauncher:/app" -w /app \
          -v "$PIP_CACHE:/root/.cache/pip" \
          -e API_KEY=test -e ZAPPY_SERVER_HOST=server -e ZAPPY_SERVER_PORT=4242 python:trixie \
          bash -c "pip install -q -r requirements-dev.txt && python tests/scene_check.py village-gathering" 2>&1)
    code=$?
    echo "$out" | grep -vE "^\[agent|^\[runtime|WARNING" | tail -6
    record "Playbook scene" $code \
           "$(echo "$out" | grep -E 'steps,' | tail -1 | xargs)"
fi

# --- Summary --------------------------------------------------------------------

echo
echo "${BOLD}================== TEST SUMMARY ==================${RESET}"
overall=0
for i in "${!CAT_NAME[@]}"; do
    printf "%-26s %-16s %s\n" "${CAT_NAME[$i]}" "${CAT_RESULT[$i]}" "${CAT_SUMMARY[$i]}"
    [[ "${CAT_RESULT[$i]}" == *FAIL* ]] && overall=1
done
echo "${BOLD}==================================================${RESET}"
if [ $overall -eq 0 ]; then
    echo "${GREEN}${BOLD}ALL CATEGORIES PASSED${RESET}"
else
    echo "${RED}${BOLD}SOME CATEGORIES FAILED${RESET}"
fi
exit $overall
