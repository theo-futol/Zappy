import asyncio
import random
import math
from src.parser import Parser
from src.ui import UI
from src.client_handler import ClientHandler
from src.player import Player

class Server:
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
        
        self.paused = False
        self.running = True
        self.ticks = 0.0
        
        self.map = [[{"food": 0, "linemate": 0, "deraumere": 0, "sibur": 0, "mendiane": 0, "phiras": 0, "thystame": 0, "players": []} for _ in range(self.width)] for _ in range(self.height)]
        self.eggs = []
        for t in self.teams:
            for _ in range(self.initial_clients):
                self.eggs.append({"team": t, "x": random.randint(0, self.width-1), "y": random.randint(0, self.height-1)})
                
        self.players = []
        self.client_handlers = {}
        self.next_client_id = 1
        
        self.ui = UI(self)
        self.spawn_resources()
        self.resource_timer = 20.0

    def spawn_resources(self):
        densities = {"food": 0.5, "linemate": 0.3, "deraumere": 0.15, "sibur": 0.1, "mendiane": 0.1, "phiras": 0.08, "thystame": 0.05}
        total_tiles = self.width * self.height
        for r, d in densities.items():
            target = int(total_tiles * d)
            current = sum(self.map[y][x][r] for y in range(self.height) for x in range(self.width))
            for _ in range(target - current):
                x, y = random.randint(0, self.width-1), random.randint(0, self.height-1)
                self.map[y][x][r] += 1

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

    def validate_incantation(self, x, y, level):
        if level not in self.ELEVATION_REQS: return False
        reqs = self.ELEVATION_REQS[level]
        tile = self.map[y][x]
        
        players_on_tile = [p for p in tile["players"] if p.level == level]
        if len(players_on_tile) < reqs["players"]: return False
        
        for k in ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]:
            if tile[k] < reqs[k]: return False
        return True

    def broadcast_message(self, sender, text):
        for p in self.players:
            if p == sender: continue
            k = 0 # Dummy direction
            if p.client_id in self.client_handlers:
                self.client_handlers[p.client_id].send_response(f"message {k}, {text}\n")

    async def handle_client(self, reader, writer):
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
                self.players.remove(handler.player)
                handler.player.is_dead = True
                if handler.player in self.map[handler.player.y][handler.player.x]["players"]:
                    self.map[handler.player.y][handler.player.x]["players"].remove(handler.player)
            if cid in self.client_handlers:
                del self.client_handlers[cid]

    async def main_loop(self):
        self.ui.init()
        server = await asyncio.start_server(self.handle_client, '0.0.0.0', self.config["port"])
        
        while self.running:
            dt = self.ui.handle_events()
            
            if not self.paused:
                virtual_dt = dt * self.freq
                self.ticks += virtual_dt
                self.resource_timer -= virtual_dt
                
                if self.resource_timer <= 0:
                    self.spawn_resources()
                    self.resource_timer = 20.0
                    
                for p in list(self.players):
                    p.update(virtual_dt)
                    if p.is_dead:
                        if p.client_id in self.client_handlers:
                            self.client_handlers[p.client_id].send_response("dead\n")
                            self.client_handlers[p.client_id].writer.close()
                        self.players.remove(p)
            
            self.ui.render()
            await asyncio.sleep(0.001)
            
        self.ui.quit()
        server.close()
        await server.wait_closed()
