# Dev stack for the Zappy VR viewer.
#
#   make            start everything (server + ai + bridge + quest tunnel)
#   make stop       stop everything
#   make server     start zappy_server alone (auto-restarts if it exits)
#   make ai         start the AI client launcher alone
#   make bridge     start the web bridge alone (auto-restarts, waits for server)
#   make tunnel     route the Quest's localhost:8081 to this Mac (USB)
#   make status     show what is running
#   make logs       tail the last lines of every log
#   make fclean     stop everything and wipe logs/pid files
#   make re         fclean, then start everything again
#
# Tunables: make FREQ=100 WIDTH=20 HEIGHT=20 AI_PER_TEAM=3 ...

REPO     := /Users/theo/again/G-YEP-400-RUN-4-1-zappy-1
UNITY    := /Applications/Unity/Hub/Editor/6000.3.19f1
NODE     := $(UNITY)/PlaybackEngines/WebGLSupport/BuildTools/Emscripten/node/node
ADB      := $(UNITY)/PlaybackEngines/AndroidPlayer/SDK/platform-tools/adb
RUN      := .run

PORT        := 4242
GUI_PORT    := 8081
WIDTH       := 15
HEIGHT      := 15
TEAMS       := team1 team2
CLIENTS     := 10
FREQ        := 20
AI_PER_TEAM := 4

.PHONY: all start stop server ai bridge tunnel status logs fclean re

all: start

start: server ai bridge tunnel
	@echo "-- stack up: connect the headset app to localhost:$(GUI_PORT)"

$(REPO)/zappy_server:
	$(MAKE) -C $(REPO) zappy_server

server: $(REPO)/zappy_server
	@mkdir -p $(RUN)
	@if pgrep -f '[z]appy_server' >/dev/null; then echo "server: already running"; else \
	  nohup bash -c 'while true; do "$(REPO)/zappy_server" -p $(PORT) -x $(WIDTH) -y $(HEIGHT) -n $(TEAMS) -c $(CLIENTS) -f $(FREQ); echo "[supervisor] server exited, restart in 1s"; sleep 1; done' >> $(RUN)/server.log 2>&1 & \
	  echo $$! > $(RUN)/server.pid; echo "server: started on port $(PORT) (supervised)"; fi

ai:
	@mkdir -p $(RUN)
	@if pgrep -f '[l]auncher.py' >/dev/null; then echo "ai: already running"; else \
	  nohup python3 "$(REPO)/src/ia/launcher.py" --host localhost --port $(PORT) --teams $(TEAMS) --target-per-team $(AI_PER_TEAM) >> $(RUN)/ai.log 2>&1 & \
	  echo $$! > $(RUN)/ai.pid; echo "ai: launcher started ($(AI_PER_TEAM) per team, retries on its own)"; fi

bridge:
	@mkdir -p $(RUN)
	@if pgrep -f '[b]ridge.js' >/dev/null; then echo "bridge: already running"; else \
	  nohup bash -c 'cd "$(REPO)/bonus/GUI/bridge" && while true; do ZAPPY_SERVER_PORT=$(PORT) GUI_PORT=$(GUI_PORT) "$(NODE)" bridge.js; echo "[supervisor] bridge exited, restart in 2s"; sleep 2; done' >> $(RUN)/bridge.log 2>&1 & \
	  echo $$! > $(RUN)/bridge.pid; echo "bridge: started on port $(GUI_PORT) (supervised)"; fi

tunnel:
	@"$(ADB)" reverse tcp:$(GUI_PORT) tcp:$(GUI_PORT) >/dev/null 2>&1 \
	  && echo "tunnel: quest localhost:$(GUI_PORT) -> mac" \
	  || echo "tunnel: no quest on USB (plug it in, then 'make tunnel')"

stop:
	@pkill -f '[z]appy_server' 2>/dev/null && echo "server: stopped" || echo "server: not running"
	@pkill -f '[b]ridge.js'    2>/dev/null && echo "bridge: stopped" || echo "bridge: not running"
	@pkill -f '[l]auncher.py'  2>/dev/null && echo "ai: stopped"     || echo "ai: not running"
	@pkill -f 'ia[/]client.py' 2>/dev/null || true
	@"$(ADB)" reverse --remove tcp:$(GUI_PORT) >/dev/null 2>&1 || true
	@rm -f $(RUN)/*.pid

fclean: stop
	@rm -rf $(RUN)
	@echo "fclean: logs and pid files removed"

re: fclean start

status:
	@pgrep -fl 'zappy_server|bridge\.js|launcher\.py' | grep -v pgrep || echo "nothing running"
	@"$(ADB)" reverse --list 2>/dev/null | grep -q tcp:$(GUI_PORT) && echo "tunnel: active" || echo "tunnel: down"

logs:
	@for f in $(RUN)/*.log; do echo "== $$f =="; tail -n 12 "$$f"; done
