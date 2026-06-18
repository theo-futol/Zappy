import pygame


class UI:
    TILE_SIZE = 50
    LEFT_PANEL_WIDTH = 300
    RIGHT_PANEL_WIDTH = 360
    MIN_HEIGHT = 620
    MAX_DT = 0.1

    def __init__(self, server):
        self.server = server
        self.screen = None
        self.font_small = None
        self.font = None
        self.font_large = None
        self.clock = None
        self.selected_tile = None
        self.colors = {
            "food": (100, 255, 100),
            "linemate": (200, 200, 200),
            "deraumere": (255, 100, 255),
            "sibur": (100, 255, 255),
            "mendiane": (255, 255, 100),
            "phiras": (255, 150, 50),
            "thystame": (255, 50, 50),
            "bg": (18, 18, 22),
            "panel": (30, 32, 38),
            "panel_alt": (44, 46, 54),
            "grid": (70, 74, 82),
            "text": (242, 242, 242),
            "muted": (180, 182, 190),
            "selected": (255, 225, 110),
            "paused": (255, 190, 80),
            "incantation_success": (80, 255, 140),
            "incantation_failed": (255, 110, 110),
            "overlay": (8, 8, 12, 150),
            "badge": (10, 10, 14),
            "crowd": (235, 240, 255, 120),
        }
        self.team_palette = [
            (0, 200, 0),
            (70, 160, 255),
            (255, 130, 60),
            (230, 90, 180),
            (240, 220, 70),
            (90, 220, 220),
        ]
        self.team_colors = {}

    def init(self):
        pygame.init()
        window_width = (
            self.LEFT_PANEL_WIDTH
            + self.server.width * self.TILE_SIZE
            + self.RIGHT_PANEL_WIDTH
        )
        window_height = max(self.server.height * self.TILE_SIZE, self.MIN_HEIGHT)
        self.screen = pygame.display.set_mode((window_width, window_height))
        pygame.display.set_caption("Zappy Lab Environment")
        self.font_small = pygame.font.SysFont("Menlo", 13)
        self.font = pygame.font.SysFont("Menlo", 16)
        self.font_large = pygame.font.SysFont("Menlo", 28, bold=True)
        self.clock = pygame.time.Clock()

    def handle_events(self):
        dt = min(self.clock.tick(60) / 1000.0, self.MAX_DT)
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.server.running = False
            elif event.type == pygame.KEYDOWN and not getattr(event, "repeat", False):
                if event.key in (pygame.K_p, pygame.K_SPACE):
                    self.server.paused = not self.server.paused
            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                x, y = event.pos
                grid_x = (x - self.LEFT_PANEL_WIDTH) // self.TILE_SIZE
                grid_y = y // self.TILE_SIZE
                if 0 <= grid_x < self.server.width and 0 <= grid_y < self.server.height:
                    self.selected_tile = (grid_x, grid_y)
        return dt

    def render(self):
        self.screen.fill(self.colors["bg"])
        self._draw_broadcast_panel()

        for y in range(self.server.height):
            for x in range(self.server.width):
                self._draw_tile(x, y)

        self._draw_side_panel()
        if self.server.paused:
            self._draw_pause_overlay()
        pygame.display.flip()

    def quit(self):
        pygame.quit()

    def _draw_tile(self, x, y):
        left = self._map_origin_x() + x * self.TILE_SIZE
        top = y * self.TILE_SIZE
        rect = pygame.Rect(left, top, self.TILE_SIZE, self.TILE_SIZE)
        pygame.draw.rect(self.screen, self.colors["grid"], rect, 1)

        event = self._event_for_tile(x, y)
        if event is not None:
            color = self.colors[event["kind"]]
            pygame.draw.rect(self.screen, color, rect, 3)
            label = self.font_small.render(str(event["label"]), True, color)
            self.screen.blit(label, (left + 3, top + self.TILE_SIZE - 16))

        if self.selected_tile == (x, y):
            pygame.draw.rect(self.screen, self.colors["selected"], rect, 3)

        tile = self.server.map[y][x]
        self._draw_resources(left, top, tile)
        self._draw_players(left, top, tile["players"])

    def _draw_resources(self, left, top, tile):
        row = 0
        for resource_name, count in tile.items():
            if resource_name == "players" or count <= 0:
                continue
            pygame.draw.rect(
                self.screen,
                self.colors.get(resource_name, (255, 255, 255)),
                (left + 3, top + 3 + row * 10, 8, 8),
            )
            text = self.font_small.render(f"x{count}", True, self.colors["text"])
            self.screen.blit(text, (left + 14, top + 1 + row * 10))
            row += 1

    def _draw_players(self, left, top, players):
        if not players:
            return

        # Players should feel like a blurry crowd occupying the whole tile.
        crowd_surface = pygame.Surface((self.TILE_SIZE - 8, self.TILE_SIZE - 8), pygame.SRCALPHA)
        dominant_color = self._crowd_color(players)
        crowd_surface.fill((*dominant_color, 110))
        self.screen.blit(crowd_surface, (left + 4, top + 4))

        pygame.draw.rect(
            self.screen,
            dominant_color,
            (left + 4, top + 4, self.TILE_SIZE - 8, self.TILE_SIZE - 8),
            2,
            border_radius=8,
        )

        count_badge = pygame.Rect(left + self.TILE_SIZE - 20, top + 4, 16, 14)
        pygame.draw.rect(self.screen, self.colors["badge"], count_badge, border_radius=3)
        count_text = self.font_small.render(str(len(players)), True, self.colors["text"])
        self.screen.blit(count_text, count_text.get_rect(center=count_badge.center))

        level_summary = self._level_summary(players)
        level_badge = pygame.Rect(left + 4, top + self.TILE_SIZE - 18, 24, 14)
        pygame.draw.rect(self.screen, self.colors["badge"], level_badge, border_radius=3)
        level_text = self.font_small.render(level_summary, True, self.colors["text"])
        self.screen.blit(level_text, level_text.get_rect(center=level_badge.center))

        if len(players) > 1:
            overlap_text = self.font_small.render("mix", True, self.colors["text"])
            self.screen.blit(overlap_text, (left + 15, top + 6))

    def _draw_broadcast_panel(self):
        panel_width = self.LEFT_PANEL_WIDTH
        panel_height = self.screen.get_height()
        pygame.draw.rect(self.screen, self.colors["panel_alt"], (0, 0, panel_width, panel_height))

        y = 18
        y = self._draw_text("Recent Broadcasts", 18, y, self.font_large)
        y += 6

        broadcasts = list(reversed(self.server.recent_broadcasts[-8:]))
        if not broadcasts:
            self._draw_text("none", 18, y, self.font, self.colors["muted"])
            return

        for broadcast in broadcasts:
            team_color = self._team_color(broadcast["team"])
            header = (
                f"#{broadcast['sender_id']} {broadcast['team']} "
                f"L{broadcast['level']} @ ({broadcast['x']}, {broadcast['y']})"
            )
            y = self._draw_text(header, 18, y, self.font_small, team_color)
            y = self._draw_wrapped_text(
                str(broadcast["text"]),
                18,
                y,
                panel_width - 36,
                self.font_small,
                self.colors["text"],
            )
            y += 8

    def _draw_side_panel(self):
        panel_x = self._map_origin_x() + self.server.width * self.TILE_SIZE
        panel_width = self.screen.get_width() - panel_x
        panel_height = self.screen.get_height()
        pygame.draw.rect(self.screen, self.colors["panel"], (panel_x, 0, panel_width, panel_height))

        y = 18
        y = self._draw_text("Server State", panel_x + 18, y, self.font_large)
        y += 6
        status_color = self.colors["paused"] if self.server.paused else self.colors["text"]
        pause_text = "PAUSED" if self.server.paused else "RUNNING"
        y = self._draw_text(pause_text, panel_x + 18, y, self.font, status_color)
        y = self._draw_text("P or SPACE: toggle pause", panel_x + 18, y, self.font_small, self.colors["muted"])
        y += 8
        y = self._draw_text(f"Freq: {self.server.freq}", panel_x + 18, y, self.font)
        y = self._draw_text(f"Ticks: {self.server.ticks:.2f}", panel_x + 18, y, self.font)
        y = self._draw_text(f"Players: {len(self.server.players)}", panel_x + 18, y, self.font)

        y += 12
        y = self._draw_text("Recent Incantations", panel_x + 18, y, self.font)
        recent_events = list(reversed(self.server.visual_events[-4:]))
        if recent_events:
            for event in recent_events:
                color = self.colors[event["kind"]]
                summary = f"({event['x']}, {event['y']}): {event['label']}"
                y = self._draw_text(summary, panel_x + 18, y, self.font_small, color)
        else:
            y = self._draw_text("none", panel_x + 18, y, self.font_small, self.colors["muted"])

        y += 12
        if self.selected_tile is None:
            self._draw_text("Click a tile to inspect it.", panel_x + 18, y, self.font, self.colors["muted"])
            return

        tx, ty = self.selected_tile
        tile = self.server.map[ty][tx]
        y = self._draw_text(f"Tile ({tx}, {ty})", panel_x + 18, y, self.font)

        for resource_name, count in tile.items():
            if resource_name == "players":
                continue
            y = self._draw_text(
                f"{resource_name}: {count}",
                panel_x + 18,
                y,
                self.font_small,
                self.colors.get(resource_name, self.colors["text"]),
            )

        players = tile["players"]
        y += 10
        y = self._draw_text(f"Players on tile: {len(players)}", panel_x + 18, y, self.font)
        if not players:
            self._draw_text("none", panel_x + 18, y, self.font_small, self.colors["muted"])
            return

        for player in players:
            y += 6
            team_color = self._team_color(player.team)
            available_slots = self.server.get_available_slots(player.team)
            y = self._draw_text(
                f"#{player.client_id} {player.team}",
                panel_x + 18,
                y,
                self.font_small,
                team_color,
            )
            y = self._draw_text(
                f"Level {player.level} | Food {player.food} | Queue {len(player.action_queue)}",
                panel_x + 18,
                y,
                self.font_small,
            )
            y = self._draw_text(
                f"Available slots: {available_slots}",
                panel_x + 18,
                y,
                self.font_small,
                self.colors["muted"],
            )
            y = self._draw_text(
                f"Starve in: {player.starvation_timer:.2f} t",
                panel_x + 18,
                y,
                self.font_small,
                self.colors["muted"],
            )
            inventory_summary = " ".join(f"{name}:{count}" for name, count in player.inventory.items())
            y = self._draw_text(
                inventory_summary,
                panel_x + 18,
                y,
                self.font_small,
                self.colors["muted"],
            )

    def _draw_pause_overlay(self):
        overlay = pygame.Surface(
            (self.server.width * self.TILE_SIZE, self.server.height * self.TILE_SIZE),
            pygame.SRCALPHA,
        )
        overlay.fill(self.colors["overlay"])
        self.screen.blit(overlay, (self._map_origin_x(), 0))

        pause_text = self.font_large.render("PAUSED", True, self.colors["paused"])
        hint_text = self.font.render("Press P or SPACE to resume", True, self.colors["text"])
        pause_rect = pause_text.get_rect(
            center=(
                self._map_origin_x() + self.server.width * self.TILE_SIZE // 2,
                self.server.height * self.TILE_SIZE // 2 - 14,
            )
        )
        hint_rect = hint_text.get_rect(
            center=(
                self._map_origin_x() + self.server.width * self.TILE_SIZE // 2,
                self.server.height * self.TILE_SIZE // 2 + 18,
            )
        )
        self.screen.blit(pause_text, pause_rect)
        self.screen.blit(hint_text, hint_rect)

    def _event_for_tile(self, x, y):
        for event in reversed(self.server.visual_events):
            if event["x"] == x and event["y"] == y:
                return event
        return None

    def _team_color(self, team_name):
        if team_name not in self.team_colors:
            color = self.team_palette[len(self.team_colors) % len(self.team_palette)]
            self.team_colors[team_name] = color
        return self.team_colors[team_name]

    def _crowd_color(self, players):
        if not players:
            return self.colors["text"]

        total_r = 0
        total_g = 0
        total_b = 0
        for player in players:
            color = self._team_color(player.team)
            total_r += color[0]
            total_g += color[1]
            total_b += color[2]

        count = len(players)
        return (total_r // count, total_g // count, total_b // count)

    def _level_summary(self, players):
        levels = sorted({player.level for player in players})
        if len(levels) == 1:
            return f"L{levels[0]}"
        return f"L{levels[0]}+"

    def _draw_text(self, text, x, y, font, color=None):
        surf = font.render(text, True, color or self.colors["text"])
        self.screen.blit(surf, (x, y))
        return y + surf.get_height() + 4

    def _draw_wrapped_text(self, text, x, y, width, font, color=None):
        words = text.split()
        if not words:
            return self._draw_text("", x, y, font, color)

        line = words[0]
        current_y = y
        for word in words[1:]:
            candidate = f"{line} {word}"
            if font.size(candidate)[0] <= width:
                line = candidate
                continue
            current_y = self._draw_text(line, x, current_y, font, color)
            line = word

        return self._draw_text(line, x, current_y, font, color)

    def _map_origin_x(self):
        return self.LEFT_PANEL_WIDTH
