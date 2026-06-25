"""Small helpers used by the heuristic model."""

from __future__ import annotations

from collections import Counter
from typing import Dict, Mapping, Sequence


LEVEL_REQUIREMENTS: Dict[int, Dict[str, object]] = {
    1: {"players": 1, "stones": {"linemate": 1}},
    2: {"players": 2, "stones": {"linemate": 1, "deraumere": 1, "sibur": 1}},
    3: {"players": 2, "stones": {"linemate": 2, "sibur": 1, "phiras": 2}},
    4: {"players": 4, "stones": {"linemate": 1, "deraumere": 1, "sibur": 2, "phiras": 1}},
    5: {"players": 4, "stones": {"linemate": 1, "deraumere": 2, "sibur": 1, "mendiane": 3}},
    6: {"players": 6, "stones": {"linemate": 1, "deraumere": 2, "sibur": 3, "phiras": 1}},
    7: {
        "players": 6,
        "stones": {
            "linemate": 2,
            "deraumere": 2,
            "sibur": 2,
            "mendiane": 2,
            "phiras": 2,
            "thystame": 1,
        },
    },
}

STONE_PRIORITY = [
    "thystame",
    "phiras",
    "mendiane",
    "sibur",
    "deraumere",
    "linemate",
]

RESOURCE_NAMES_WITHOUT_FOOD = tuple(reversed(STONE_PRIORITY))


def normalize_inventory(inventory: Mapping[str, int]) -> dict[str, int]:
    normalized = {name: 0 for name in ["food", *RESOURCE_NAMES_WITHOUT_FOOD]}
    for name, quantity in inventory.items():
        normalized[str(name)] = int(quantity)
    return normalized


def normalize_visible_tiles(
    visible_tiles: Sequence[Mapping[str, object]],
) -> list[dict[str, object]]:
    normalized_tiles: list[dict[str, object]] = []
    for index, tile in enumerate(visible_tiles):
        items = tile.get("items", [])
        if not isinstance(items, Sequence) or isinstance(items, (str, bytes)):
            raise TypeError("Chaque case doit contenir une liste 'items'.")
        normalized_tiles.append(
            {
                "index": int(tile.get("index", index)),
                "x": int(tile.get("x", 0)),
                "y": int(tile.get("y", 0)),
                "distance": int(tile.get("distance", 0)),
                "items": [str(item) for item in items],
            }
        )
    return normalized_tiles


def find_current_tile(visible_tiles: Sequence[Mapping[str, object]]) -> Mapping[str, object]:
    for tile in visible_tiles:
        if int(tile["distance"]) == 0:
            return tile
    return visible_tiles[0]


def ground_matches_requirement(
    current_counts: Counter[str],
    needed_on_ground: Mapping[str, int],
) -> bool:
    return all(current_counts.get(stone, 0) >= amount for stone, amount in needed_on_ground.items())


def missing_players_for_incantation(level: int, current_counts: Counter[str]) -> int:
    requirement = LEVEL_REQUIREMENTS.get(level)
    if requirement is None:
        return 0
    players_needed = int(requirement["players"])
    players_here = current_counts.get("player", 0)
    return max(0, players_needed - players_here)


def needed_stones_for_level(level: int, inventory: Mapping[str, int]) -> set[str]:
    requirement = LEVEL_REQUIREMENTS.get(level)
    if requirement is None:
        return set()

    missing = set()
    for stone, amount in requirement["stones"].items():
        if inventory.get(stone, 0) < amount:
            missing.add(stone)
    return missing


def pick_current_tile_resource(
    current_counts: Counter[str],
    candidates: Sequence[str] | set[str],
) -> str | None:
    for resource in STONE_PRIORITY:
        if resource in candidates and current_counts.get(resource, 0) > 0:
            return resource
    if "food" in candidates and current_counts.get("food", 0) > 0:
        return "food"
    return None


def select_visible_target(
    visible_tiles: Sequence[Mapping[str, object]],
    target_resources: set[str],
) -> Mapping[str, object] | None:
    best: tuple[int, int, int] | None = None
    best_tile: Mapping[str, object] | None = None

    for tile in visible_tiles:
        if int(tile["distance"]) == 0:
            continue
        resources = set(tile["items"])
        if not (resources & target_resources):
            continue

        dx = int(tile["x"])
        distance = int(tile["distance"])
        lateral_penalty = abs(dx)
        candidate = (distance, lateral_penalty, int(tile["index"]))
        if best is None or candidate < best:
            best = candidate
            best_tile = tile

    return best_tile


def build_plan_to_tile(target: Mapping[str, object]) -> tuple[str, ...]:
    dx = int(target["x"])
    dy = int(target["y"])
    commands: list[str] = []

    # Move forward first, then adjust left or right on the last row.
    if dy > 0:
        commands.extend(["Forward"] * dy)

    if dx < 0:
        commands.append("Left")
        commands.extend(["Forward"] * abs(dx))
    elif dx > 0:
        commands.append("Right")
        commands.extend(["Forward"] * abs(dx))

    if not commands:
        commands.append("Forward")
    return tuple(commands)
