import random

from src.commands import CommandConfig

class Player:
    def __init__(self, client_id, team_name, x, y, server):
        self.client_id = client_id
        self.team = team_name
        self.x = x
        self.y = y
        self.server = server
        self.level = 1
        self.food = 10
        self.inventory = {
            "linemate": 0, "deraumere": 0, "sibur": 0,
            "mendiane": 0, "phiras": 0, "thystame": 0
        }
        self.direction = random.randint(1, 4) # 1:N, 2:E, 3:S, 4:W
        self.action_queue = [] # tuple (command_str, ticks_remaining)
        self.is_dead = False
        self.starvation_timer = 126 # ticks
        self.is_frozen = False
        
        self.server.map[y][x]["players"].append(self)

    def enqueue_command(self, cmd_str):
        if len(self.action_queue) < 10:
            parts = cmd_str.split()
            cmd = parts[0] if parts else ""
            cost = CommandConfig.COSTS.get(cmd, None)
            if cost is not None:
                self.action_queue.append((cmd_str, cost))
            else:
                self.action_queue.append((cmd_str, 0)) # Will return ko immediately

    def update(self, virtual_dt):
        if self.is_dead:
            return
            
        self.starvation_timer -= virtual_dt
        while self.starvation_timer <= 0:
            self.food -= 1
            if self.food <= 0:
                self.is_dead = True
                self.server.map[self.y][self.x]["players"].remove(self)
                return
            self.starvation_timer += 126

        if self.is_frozen or not self.action_queue:
            return

        cmd_str, ticks_left = self.action_queue[0]
        ticks_left -= virtual_dt
        
        if ticks_left <= 0:
            self.action_queue.pop(0)
            response = self.execute_command(cmd_str)
            if self.client_id in self.server.client_handlers:
                self.server.client_handlers[self.client_id].send_response(response)
        else:
            self.action_queue[0] = (cmd_str, ticks_left)

    def execute_command(self, cmd_str):
        parts = cmd_str.split()
        cmd = parts[0] if parts else ""
        args = parts[1:]

        if cmd == "Forward": return self.Forward()
        if cmd == "Right": return self.Right()
        if cmd == "Left": return self.Left()
        if cmd == "Look": return self.Look()
        if cmd == "Inventory": return self.Inventory()
        if cmd == "Broadcast": return self.Broadcast(" ".join(args))
        if cmd == "Connect_nbr": return self.Connect_nbr()
        if cmd == "Fork": return self.Fork()
        if cmd == "Eject": return self.Eject()
        if cmd == "Take": return self.Take(args[0] if args else "")
        if cmd == "Set": return self.Set(args[0] if args else "")
        if cmd == "Incantation": return self.Incantation()
        
        return "ko\n"

    def Forward(self):
        self.server.map[self.y][self.x]["players"].remove(self)
        if self.direction == 1: self.y = (self.y - 1) % self.server.height
        elif self.direction == 2: self.x = (self.x + 1) % self.server.width
        elif self.direction == 3: self.y = (self.y + 1) % self.server.height
        elif self.direction == 4: self.x = (self.x - 1) % self.server.width
        self.server.map[self.y][self.x]["players"].append(self)
        return "ok\n"

    def Right(self):
        self.direction = (self.direction % 4) + 1
        return "ok\n"

    def Left(self):
        self.direction = ((self.direction - 2) % 4) + 1
        return "ok\n"

    def Look(self):
        res = []
        for i in range(self.level + 1):
            for j in range(-i, i + 1):
                tx, ty = self.x, self.y
                if self.direction == 1:
                    tx += j; ty -= i
                elif self.direction == 2:
                    tx += i; ty += j
                elif self.direction == 3:
                    tx -= j; ty += i
                elif self.direction == 4:
                    tx -= i; ty -= j
                
                tx %= self.server.width
                ty %= self.server.height
                
                tile_contents = []
                tile = self.server.map[ty][tx]
                for p in tile["players"]: tile_contents.append("player")
                for r, c in tile.items():
                    if r != "players" and c > 0:
                        tile_contents.extend([r] * c)
                res.append(" ".join(tile_contents))
        return "[" + ",".join(res) + "]\n"

    def Inventory(self):
        inv = f"[food {self.food}, linemate {self.inventory['linemate']}, deraumere {self.inventory['deraumere']}, sibur {self.inventory['sibur']}, mendiane {self.inventory['mendiane']}, phiras {self.inventory['phiras']}, thystame {self.inventory['thystame']}]\n"
        return inv

    def Broadcast(self, text):
        self.server.broadcast_message(self, text)
        return "ok\n"

    def Connect_nbr(self):
        return f"{self.server.get_available_slots(self.team)}\n"

    def Fork(self):
        self.server.eggs.append({"team": self.team, "x": self.x, "y": self.y})
        return "ok\n"

    def Eject(self):
        tile = self.server.map[self.y][self.x]
        pushed = False
        for p in list(tile["players"]):
            if p != self:
                p.direction = self.direction
                p.Forward()
                k = 1 # Simplified K direction
                if p.client_id in self.server.client_handlers:
                    self.server.client_handlers[p.client_id].send_response(f"eject: {k}\n")
                pushed = True
        return "ok\n" if pushed else "ko\n"

    def Take(self, obj):
        if obj == "food":
            if self.server.map[self.y][self.x]["food"] > 0:
                self.server.map[self.y][self.x]["food"] -= 1
                self.food += 1
                return "ok\n"
        elif obj in self.inventory:
            if self.server.map[self.y][self.x][obj] > 0:
                self.server.map[self.y][self.x][obj] -= 1
                self.inventory[obj] += 1
                return "ok\n"
        return "ko\n"

    def Set(self, obj):
        if obj == "food":
            if self.food > 0:
                self.food -= 1
                self.server.map[self.y][self.x]["food"] += 1
                return "ok\n"
        elif obj in self.inventory:
            if self.inventory[obj] > 0:
                self.inventory[obj] -= 1
                self.server.map[self.y][self.x][obj] += 1
                return "ok\n"
        return "ko\n"

    def Incantation(self):
        ritual_level = self.level
        participants = [
            player
            for player in self.server.map[self.y][self.x]["players"]
            if player.level == ritual_level
        ]

        if not self.server.validate_incantation(self.x, self.y, ritual_level):
            self.server.add_visual_event(
                "incantation_failed",
                self.x,
                self.y,
                f"L{ritual_level} KO",
            )
            return "ko\n"

        for player in participants:
            player.level = ritual_level + 1
            if player.client_id in self.server.client_handlers and player != self:
                self.server.client_handlers[player.client_id].send_response(
                    f"Current level: {player.level}\n"
                )

        reqs = self.server.ELEVATION_REQS[ritual_level]
        for k in ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]:
            self.server.map[self.y][self.x][k] -= reqs[k]

        self.server.add_visual_event(
            "incantation_success",
            self.x,
            self.y,
            f"L{ritual_level}->{ritual_level + 1} x{len(participants)}",
        )
        return f"Current level: {ritual_level + 1}\n"
