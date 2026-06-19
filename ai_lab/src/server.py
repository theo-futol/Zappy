import asyncio
import math
import random
from collections import Counter
from src.parser import Parser
from src.client_handler import ClientHandler
from src.player import Player

try:
    from src.ui import UI
except Exception:
    from src.headless_ui import HeadlessUI as UI

class Server:
    MAX_LEVEL = 8
    WINNER_PLAYER_COUNT = 6
    GAME_OVER_DISPLAY_SECONDS = 3.0
    RESOURCE_RESPAWN_INTERVAL = 20.0
    RULES_PROFILE_SUBJECT = "subject"
    RULES_PROFILE_ORIGIN_SERVER_MAIN = "origin_server_main"
    SUBJECT_RESOURCE_DENSITIES = {
        "food": 0.5,
        "linemate": 0.3,
        "deraumere": 0.15,
        "sibur": 0.1,
        "mendiane": 0.1,
        "phiras": 0.08,
        "thystame": 0.05,
    }
    ORIGIN_SERVER_MAIN_RESOURCE_DENSITIES = {
        "food": 0.5,
        "linemate": 0.3,
        "deraumere": 0.5,
        "sibur": 0.1,
        "mendiane": 0.1,
        "phiras": 0.08,
        "thystame": 0.05,
    }
    RESOURCE_NAMES = tuple(SUBJECT_RESOURCE_DENSITIES)

    ELEVATION_REQS = {
        1: {"players": 1, "linemate": 1, "deraumere": 0, "sibur": 0, "mendiane": 0, "phiras": 0, "thystame": 0},
        2: {"players": 2, "linemate": 1, "deraumere": 1, "sibur": 1, "mendiane": 0, "phiras": 0, "thystame": 0},
        3: {"players": 2, "linemate": 2, "deraumere": 0, "sibur": 1, "mendiane": 0, "phiras": 2, "thystame": 0},
        4: {"players": 4, "linemate": 1, "deraumere": 1, "sibur": 2, "mendiane": 0, "phiras": 1, "thystame": 0},
        5: {"players": 4, "linemate": 1, "deraumere": 2, "sibur": 1, "mendiane": 3, "phiras": 0, "thystame": 0},
        6: {"players": 6, "linemate": 1, "deraumere": 2, "sibur": 3, "mendiane": 0, "phiras": 1, "thystame": 0},
        7: {"players": 6, "linemate": 2, "deraumere": 2, "sibur": 2, "mendiane": 2, "phiras": 2, "thystame": 1}
    }

    def __init__(self, config_path="config.json"):
        self.config = Parser.load_config(config_path)
        self.width = self.config["width"]
        self.height = self.config["height"]
        self.teams = self.config["teams"]
        self.freq = self.config["default_freq"]
        self.initial_clients = self.config["initial_clients_per_team"]
        self.rules_profile = str(
            self.config.get("rules_profile", self.RULES_PROFILE_ORIGIN_SERVER_MAIN)
        )
        
        self.paused = False
        self.running = True
        self.ticks = 0.0
        self.loop_time = 0.0
        self.last_resource_spawn_at = None
        
        self.map = [[{"food": 0, "linemate": 0, "deraumere": 0, "sibur": 0, "mendiane": 0, "phiras": 0, "thystame": 0, "players": []} for _ in range(self.width)] for _ in range(self.height)]
        self.eggs = []
        for t in self.teams:
            for _ in range(self.initial_clients):
                self.eggs.append({"team": t, "x": random.randint(0, self.width-1), "y": random.randint(0, self.height-1)})
                
        self.players = []
        self.client_handlers = {}
        self.next_client_id = 1
        self.visual_events = []
        self.recent_broadcasts = []
        self.active_incantations = {}
        self.tcp_server = None
        self.winner_team = None
        self.winner_player_count = 0
        self.game_over = False
        self.shutdown_deadline = None
        self.shutdown_started = False
        
        self.ui = UI(self)
        self.spawn_resources()
        self.resource_timer = self.RESOURCE_RESPAWN_INTERVAL

    def spawn_resources(self):
        if self._uses_origin_server_main_rules():
            self._spawn_resources_origin_server_main()
            return

        self._spawn_resources_subject()

    def _spawn_resources_subject(self):
        for resource_name in self.RESOURCE_NAMES:
            target_quantity = self._target_resource_quantity(resource_name)
            current_quantity = self._current_resource_quantity(resource_name)
            missing_quantity = target_quantity - current_quantity
            if missing_quantity <= 0:
                continue
            self._place_resource_evenly(resource_name, missing_quantity)

    def _spawn_resources_origin_server_main(self):
        total_tiles = self.width * self.height
        densities = self._resource_densities()
        for resource_name, density in densities.items():
            quantity_to_add = int(total_tiles * float(density))
            if quantity_to_add <= 0:
                continue
            self._place_resource_randomly(resource_name, quantity_to_add)

    def _target_resource_quantity(self, resource_name):
        total_tiles = self.width * self.height
        density = float(self._resource_densities()[resource_name])
        # The subject requires at least one unit of each resource on the floor.
        return max(1, int(total_tiles * density))

    def _current_resource_quantity(self, resource_name):
        return sum(
            self.map[y][x][resource_name]
            for y in range(self.height)
            for x in range(self.width)
        )

    def _place_resource_evenly(self, resource_name, quantity):
        positions = [(x, y) for y in range(self.height) for x in range(self.width)]
        random.shuffle(positions)
        for _ in range(quantity):
            target_x, target_y = min(
                positions,
                key=lambda position: self._resource_placement_score(
                    resource_name,
                    position[0],
                    position[1],
                ),
            )
            self.map[target_y][target_x][resource_name] += 1

    def _place_resource_randomly(self, resource_name, quantity):
        for _ in range(quantity):
            target_x = random.randint(0, self.width - 1)
            target_y = random.randint(0, self.height - 1)
            self.map[target_y][target_x][resource_name] += 1

    def _resource_placement_score(self, resource_name, x, y):
        tile = self.map[y][x]
        total_resources = sum(tile[name] for name in self.RESOURCE_NAMES)
        return (
            tile[resource_name],
            total_resources,
        )

    def _resource_densities(self):
        if self._uses_origin_server_main_rules():
            return self.ORIGIN_SERVER_MAIN_RESOURCE_DENSITIES
        return self.SUBJECT_RESOURCE_DENSITIES

    def _uses_origin_server_main_rules(self):
        return self.rules_profile == self.RULES_PROFILE_ORIGIN_SERVER_MAIN

    def get_available_slots(self, team):
        return sum(1 for egg in self.eggs if egg["team"] == team)

    def consume_slot(self, team):
        for i, egg in enumerate(self.eggs):
            if egg["team"] == team:
                return self.eggs.pop(i)
        return None

    def spawn_player(self, client_id, team):
        x, y = random.randint(0, self.width-1), random.randint(0, self.height-1)
        p = Player(client_id, team, x, y, self)
        self.players.append(p)
        return p

    def count_players_by_level(self):
        counts = {level: 0 for level in range(1, self.MAX_LEVEL + 1)}
        for player in self.players:
            normalized_level = max(1, min(int(player.level), self.MAX_LEVEL))
            counts[normalized_level] += 1
        return counts

    def count_max_level_players_by_team(self):
        counts = Counter()
        for player in self.players:
            if player.level >= self.MAX_LEVEL:
                counts[player.team] += 1
        return counts

    def evaluate_winner(self):
        level_counts = self.count_max_level_players_by_team()
        for team_name in self.teams:
            player_count = int(level_counts.get(team_name, 0))
            if player_count >= self.WINNER_PLAYER_COUNT:
                return team_name, player_count
        return None, 0

    def check_for_winner(self):
        if self.game_over:
            return

        winner_team, player_count = self.evaluate_winner()
        if winner_team is None:
            return

        self.winner_team = winner_team
        self.winner_player_count = player_count
        self.game_over = True
        self.paused = True
        print(
            f"[server] winner detected: {winner_team} "
            f"with {player_count} player(s) at level {self.MAX_LEVEL}"
        )

    async def begin_shutdown_if_needed(self, now):
        if not self.game_over or self.shutdown_started:
            return

        self.shutdown_started = True
        self.shutdown_deadline = now + self.GAME_OVER_DISPLAY_SECONDS

        if self.tcp_server is not None:
            self.tcp_server.close()

        await self._broadcast_game_end()

    async def _broadcast_game_end(self):
        if self.winner_team is None:
            return

        message = f"seg {self.winner_team}\n"
        handlers = list(self.client_handlers.values())
        await asyncio.gather(
            *(self._notify_handler_game_end(handler, message) for handler in handlers),
            return_exceptions=True,
        )

    async def _notify_handler_game_end(self, handler, message):
        handler.send_response(message)
        await handler.close()

    def shutdown_countdown(self, now=None):
        if now is None:
            now = self.loop_time
        if self.shutdown_deadline is None:
            return self.GAME_OVER_DISPLAY_SECONDS
        return max(0.0, self.shutdown_deadline - now)

    def validate_incantation(self, x, y, level):
        if level not in self.ELEVATION_REQS: return False
        reqs = self.ELEVATION_REQS[level]
        tile = self.map[y][x]
        
        players_on_tile = [p for p in tile["players"] if p.level == level]
        if len(players_on_tile) < reqs["players"]: return False
        
        for k in ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]:
            if tile[k] < reqs[k]: return False
        return True

    def begin_incantation(self, initiator):
        if initiator.client_id in self.active_incantations:
            return False

        ritual_level = int(initiator.level)
        if not self.validate_incantation(initiator.x, initiator.y, ritual_level):
            return False

        tile = self.map[initiator.y][initiator.x]
        participants = [
            player
            for player in tile["players"]
            if not player.is_dead and int(player.level) == ritual_level
        ]
        frozen_players = [player for player in participants if player != initiator]

        for player in frozen_players:
            player.is_frozen = True

        ritual = {
            "initiator_id": initiator.client_id,
            "level": ritual_level,
            "x": initiator.x,
            "y": initiator.y,
            "participant_ids": [player.client_id for player in participants],
            "frozen_ids": [player.client_id for player in frozen_players],
        }
        self.active_incantations[initiator.client_id] = ritual

        for player in participants:
            handler = self.client_handlers.get(player.client_id)
            if handler is not None:
                handler.send_response("Elevation underway\n")
        return True

    def complete_incantation(self, initiator):
        ritual = self.active_incantations.pop(initiator.client_id, None)
        if ritual is None:
            return "ko\n"

        ritual_level = int(ritual["level"])
        ritual_x = int(ritual["x"])
        ritual_y = int(ritual["y"])
        participant_ids = set(ritual["participant_ids"])

        participants = []
        for player in self.players:
            if player.client_id not in participant_ids:
                continue
            if player.is_dead or int(player.level) != ritual_level:
                self._release_incantation(ritual)
                self.add_visual_event(
                    "incantation_failed",
                    ritual_x,
                    ritual_y,
                    f"L{ritual_level} KO",
                )
                return "ko\n"
            if player.x != ritual_x or player.y != ritual_y:
                self._release_incantation(ritual)
                self.add_visual_event(
                    "incantation_failed",
                    ritual_x,
                    ritual_y,
                    f"L{ritual_level} KO",
                )
                return "ko\n"
            participants.append(player)

        if len(participants) != len(participant_ids):
            self._release_incantation(ritual)
            self.add_visual_event(
                "incantation_failed",
                ritual_x,
                ritual_y,
                f"L{ritual_level} KO",
            )
            return "ko\n"

        if not self.validate_incantation(ritual_x, ritual_y, ritual_level):
            self._release_incantation(ritual)
            self.add_visual_event(
                "incantation_failed",
                ritual_x,
                ritual_y,
                f"L{ritual_level} KO",
            )
            return "ko\n"

        for player in participants:
            player.level = ritual_level + 1
            if player.client_id in self.client_handlers and player != initiator:
                self.client_handlers[player.client_id].send_response(
                    f"Current level: {player.level}\n"
                )

        reqs = self.ELEVATION_REQS[ritual_level]
        for k in ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]:
            self.map[ritual_y][ritual_x][k] -= reqs[k]

        self._release_incantation(ritual)
        self.add_visual_event(
            "incantation_success",
            ritual_x,
            ritual_y,
            f"L{ritual_level}->{ritual_level + 1} x{len(participants)}",
        )
        self.check_for_winner()
        return f"Current level: {ritual_level + 1}\n"

    def cancel_incantation_for_player(self, player):
        ritual = self.active_incantations.pop(player.client_id, None)
        if ritual is None:
            return

        self._release_incantation(ritual)
        self.add_visual_event(
            "incantation_failed",
            int(ritual["x"]),
            int(ritual["y"]),
            f"L{int(ritual['level'])} KO",
        )

    def _release_incantation(self, ritual):
        for frozen_id in ritual["frozen_ids"]:
            frozen_player = self._find_player_by_id(frozen_id)
            if frozen_player is not None and not frozen_player.is_dead:
                frozen_player.is_frozen = False

    def _find_player_by_id(self, client_id):
        for player in self.players:
            if player.client_id == client_id:
                return player
        return None

    def broadcast_message(self, sender, text):
        self.recent_broadcasts.append(
            {
                "sender_id": sender.client_id,
                "team": sender.team,
                "level": sender.level,
                "x": sender.x,
                "y": sender.y,
                "text": text,
            }
        )
        self.recent_broadcasts = self.recent_broadcasts[-10:]

        for p in self.players:
            if p == sender:
                continue
            k = self.compute_sound_direction(sender, p)
            if p.client_id in self.client_handlers:
                self.client_handlers[p.client_id].send_response(f"message {k}, {text}\n")

    def compute_sound_direction(self, sender, receiver):
        dx = self._shortest_toroidal_delta(sender.x - receiver.x, self.width)
        dy = self._shortest_toroidal_delta(sender.y - receiver.y, self.height)

        if dx == 0 and dy == 0:
            return 0

        right, forward = self._world_vector_to_local(dx, dy, receiver.direction)
        angle = math.degrees(math.atan2(-right, forward))

        if -22.5 <= angle < 22.5:
            return 1
        if 22.5 <= angle < 67.5:
            return 2
        if 67.5 <= angle < 112.5:
            return 3
        if 112.5 <= angle < 157.5:
            return 4
        if angle >= 157.5 or angle < -157.5:
            return 5
        if -157.5 <= angle < -112.5:
            return 6
        if -112.5 <= angle < -67.5:
            return 7
        return 8

    def _shortest_toroidal_delta(self, delta, size):
        half_size = size / 2
        if delta > half_size:
            return delta - size
        if delta < -half_size:
            return delta + size
        return delta

    def _world_vector_to_local(self, dx, dy, direction):
        if direction == 1:
            return dx, -dy
        if direction == 2:
            return dy, dx
        if direction == 3:
            return -dx, dy
        return -dy, -dx

    def add_visual_event(self, kind, x, y, label, ttl=3.0):
        self.visual_events.append(
            {
                "kind": kind,
                "x": x,
                "y": y,
                "label": label,
                "ttl": float(ttl),
            }
        )
        self.visual_events = self.visual_events[-12:]

    def update_visual_events(self, dt):
        active_events = []
        for event in self.visual_events:
            event["ttl"] -= dt
            if event["ttl"] > 0:
                active_events.append(event)
        self.visual_events = active_events

    async def handle_client(self, reader, writer):
        if self.game_over:
            writer.close()
            await writer.wait_closed()
            return

        cid = self.next_client_id
        self.next_client_id += 1
        handler = ClientHandler(reader, writer, self, cid)
        self.client_handlers[cid] = handler
        try:
            await handler.run()
        except Exception:
            pass
        finally:
            if handler.player and handler.player in self.players:
                self.cancel_incantation_for_player(handler.player)
                self.players.remove(handler.player)
                handler.player.is_dead = True
                if handler.player in self.map[handler.player.y][handler.player.x]["players"]:
                    self.map[handler.player.y][handler.player.x]["players"].remove(handler.player)
            if cid in self.client_handlers:
                del self.client_handlers[cid]

    async def main_loop(self):
        self.ui.init()
        self.tcp_server = await asyncio.start_server(self.handle_client, '0.0.0.0', self.config["port"])

        try:
            while self.running:
                loop_time = asyncio.get_running_loop().time()
                self.loop_time = loop_time
                dt = self.ui.handle_events()

                await self.begin_shutdown_if_needed(loop_time)

                if not self.paused:
                    virtual_dt = dt * self.freq
                    self.ticks += virtual_dt
                    self.update_visual_events(dt)
                    self._update_resource_spawns(loop_time, virtual_dt)

                    for p in list(self.players):
                        p.update(virtual_dt)
                        if p.is_dead:
                            if p.client_id in self.client_handlers:
                                self.client_handlers[p.client_id].send_response("dead\n")
                                await self.client_handlers[p.client_id].close()
                            if p in self.players:
                                self.players.remove(p)

                self.ui.render()

                if self.shutdown_deadline is not None and loop_time >= self.shutdown_deadline:
                    self.running = False

                await asyncio.sleep(0.001)
        finally:
            self.ui.quit()
            await asyncio.gather(
                *(handler.close() for handler in list(self.client_handlers.values())),
                return_exceptions=True,
            )
            if self.tcp_server is not None:
                self.tcp_server.close()
                await self.tcp_server.wait_closed()

    def _update_resource_spawns(self, loop_time, virtual_dt):
        if self._uses_origin_server_main_rules():
            if self.last_resource_spawn_at is None:
                self.last_resource_spawn_at = loop_time
            while loop_time - self.last_resource_spawn_at >= self.RESOURCE_RESPAWN_INTERVAL:
                self.spawn_resources()
                self.last_resource_spawn_at += self.RESOURCE_RESPAWN_INTERVAL
            return

        self.resource_timer -= virtual_dt
        while self.resource_timer <= 0:
            self.spawn_resources()
            self.resource_timer += self.RESOURCE_RESPAWN_INTERVAL
