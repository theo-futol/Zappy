"""A single fresh snapshot of the world, rebuilt at the start of every turn."""

from __future__ import annotations

from collections import Counter

try:
    from .utils.model_utils import LEVEL_REQUIREMENTS, needed_stones_for_level
except ImportError:
    from utils.model_utils import LEVEL_REQUIREMENTS, needed_stones_for_level


def tile_positions(tile_count: int) -> list[tuple[int, int]]:
    positions = [(0, 0)]
    depth = 1
    while len(positions) < tile_count:
        for x in range(-depth, depth + 1):
            positions.append((x, depth))
            if len(positions) == tile_count:
                break
        depth += 1
    return positions[:tile_count]


def build_visible_tiles(look_tiles: list[list[str]]) -> list[dict[str, object]]:
    visible_tiles = []
    for index, (x, y) in enumerate(tile_positions(len(look_tiles))):
        visible_tiles.append(
            {
                "index": index,
                "x": x,
                "y": y,
                "distance": abs(x) + y,
                "items": list(look_tiles[index]),
            }
        )
    return visible_tiles


class Observation:
    """Everything the strategy needs to decide on one turn."""

    def __init__(self, client, level: int) -> None:
        self.dead = client.dead
        if self.dead:
            self.tiles: list[dict[str, object]] = []
            self.inv: dict[str, int] = {}
            self.food = 0
            self.current_counts: Counter[str] = Counter()
            self.needed_stones: set[str] = set()
            self.ready_for_elevation = False
            self.players_here = 0
            self.messages: list[tuple[int, str]] = []
            return

        raw_tiles = client.look()
        self.inv = client.inventory()
        self.tiles = build_visible_tiles(raw_tiles)
        self.food = int(self.inv.get("food", 0))
        self.current_counts = Counter(self.tiles[0]["items"]) if self.tiles else Counter()
        self.needed_stones = needed_stones_for_level(level, self.inv)
        self.ready_for_elevation = level in LEVEL_REQUIREMENTS and not self.needed_stones
        self.players_here = self.current_counts.get("player", 0)
        self.messages = client.pop_messages()

    def on_my_tile(self, item: str) -> bool:
        return self.current_counts.get(item, 0) > 0
