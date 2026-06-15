import pygame

class UI:
    def __init__(self, server):
        self.server = server
        self.screen = None
        self.font = None
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
            "bg": (20, 20, 20),
            "grid": (50, 50, 50),
            "text": (255, 255, 255)
        }

    def init(self):
        pygame.init()
        self.screen = pygame.display.set_mode((1000, 600))
        pygame.display.set_caption("Zappy Lab Environment")
        self.font = pygame.font.SysFont(None, 16)
        self.clock = pygame.time.Clock()

    def handle_events(self):
        dt = self.clock.tick(60) / 1000.0
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.server.running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_p:
                    self.server.paused = not self.server.paused
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1: # Left click
                    x, y = event.pos
                    grid_x = x // 50
                    grid_y = y // 50
                    if 0 <= grid_x < self.server.width and 0 <= grid_y < self.server.height:
                        self.selected_tile = (grid_x, grid_y)
        return dt

    def render(self):
        self.screen.fill(self.colors["bg"])
        
        for y in range(self.server.height):
            for x in range(self.server.width):
                rect = pygame.Rect(x * 50, y * 50, 50, 50)
                pygame.draw.rect(self.screen, self.colors["grid"], rect, 1)
                
                tile = self.server.map[y][x]
                res_y = 0
                for r, c in tile.items():
                    if r != "players" and c > 0:
                        pygame.draw.rect(self.screen, self.colors.get(r, (255,255,255)), (x*50 + 2, y*50 + 2 + res_y, 8, 8))
                        text = self.font.render(f"x{c}", True, self.colors["text"])
                        self.screen.blit(text, (x*50 + 12, y*50 + 2 + res_y))
                        res_y += 10
                        
                for p in tile["players"]:
                    px = x * 50 + 25
                    py = y * 50 + 25
                    points = []
                    if p.direction == 1: points = [(px, py-10), (px-10, py+10), (px+10, py+10)]
                    elif p.direction == 2: points = [(px+10, py), (px-10, py-10), (px-10, py+10)]
                    elif p.direction == 3: points = [(px, py+10), (px-10, py-10), (px+10, py-10)]
                    elif p.direction == 4: points = [(px-10, py), (px+10, py-10), (px+10, py+10)]
                    pygame.draw.polygon(self.screen, (0, 200, 0), points)

        pygame.draw.rect(self.screen, (30, 30, 30), (self.server.width * 50, 0, 1000 - self.server.width * 50, 600))
        y_offset = 20
        def draw_text(txt):
            nonlocal y_offset
            surf = self.font.render(txt, True, self.colors["text"])
            self.screen.blit(surf, (self.server.width * 50 + 20, y_offset))
            y_offset += 20
            
        draw_text(f"PAUSED: {self.server.paused}")
        draw_text(f"Freq: {self.server.freq}")
        draw_text(f"Ticks: {self.server.ticks:.2f}")
        draw_text(f"Clients: {len(self.server.players)}")
        
        if self.selected_tile:
            tx, ty = self.selected_tile
            draw_text(f"--- Tile ({tx}, {ty}) ---")
            tile = self.server.map[ty][tx]
            for r, c in tile.items():
                if r != "players": draw_text(f"{r}: {c}")
            draw_text(f"Players: {len(tile['players'])}")
            if tile['players']:
                p = tile['players'][0]
                draw_text(f"--- Player Context ---")
                draw_text(f"Team: {p.team}")
                draw_text(f"Level: {p.level}")
                draw_text(f"Food: {p.food}")
                draw_text(f"Starve in: {p.starvation_timer:.2f} t")
                for item, count in p.inventory.items():
                    draw_text(f"{item}: {count}")

        pygame.display.flip()

    def quit(self):
        pygame.quit()
