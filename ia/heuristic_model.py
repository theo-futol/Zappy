"""Modele heuristique simple pour un client IA Zappy."""

from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
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

SURVIVAL_FOOD_THRESHOLD = 5


@dataclass(frozen=True)
class HeuristicDecision:
    command: str
    rationale: str
    confidence: float


class HeuristicModel:

    def predict(
        self,
        *,
        level: int,
        team_name: str,
        inventory: Mapping[str, int],
        visible_tiles: Sequence[Mapping[str, object]],
        look_is_fresh: bool,
        objective: str = "exploration",
    ) -> HeuristicDecision:
        if not team_name:
            raise ValueError("team_name cannot be empty")

        normalized_inventory = _normalize_inventory(inventory)
        normalized_tiles = _normalize_visible_tiles(visible_tiles)
        if not look_is_fresh or not normalized_tiles:
            return HeuristicDecision(
                command="Look",
                rationale="vision not fresh or no visible tiles, need to update vision.",
                confidence=0.99,
            )

        current_tile = _find_current_tile(normalized_tiles)
        current_counts = Counter(current_tile["items"])

        if normalized_inventory["food"] <= SURVIVAL_FOOD_THRESHOLD:
            if current_counts["food"] > 0:
                return HeuristicDecision(
                    command="Take food",
                    rationale="food is on the current tile, need to take it to survive.",
                    confidence=0.98,
                )
            target = _select_visible_target(normalized_tiles, {"food"})
            if target is not None:
                return _move_towards(target, "food is prioritized for survival.")

        if objective == "elevation":
            ritual_command = _decide_incantation_step(level, normalized_inventory, current_counts)
            if ritual_command is not None:
                return ritual_command

            needed_stones = _needed_stones_for_level(level, normalized_inventory)
            current_stone = _pick_current_tile_resource(current_counts, needed_stones)
            if current_stone is not None:
                return HeuristicDecision(
                    command=f"Take {current_stone}",
                    rationale=f"current content {current_stone}, can be useful for the incantation.",
                    confidence=0.9,
                )

            target = _select_visible_target(normalized_tiles, needed_stones)
            if target is not None:
                return _move_towards(target, "stone needed for incantation is visible, moving towards it.")

        if current_counts["food"] > 0:
            return HeuristicDecision(
                command="Take food",
                rationale="food is on the current tile, need to take it to survive.",
                confidence=0.74,
            )

        current_stone = _pick_current_tile_resource(current_counts, STONE_PRIORITY)
        if current_stone is not None:
            return HeuristicDecision(
                command=f"Take {current_stone}",
                rationale=f"current content {current_stone}, can be useful for the incantation.",
                confidence=0.7,
            )

        target = _select_visible_target(normalized_tiles, set(STONE_PRIORITY) | {"food"})
        if target is not None:
            return _move_towards(target, "A useful resource is visible, moving towards it.")

        return HeuristicDecision(
            command="Forward",
            rationale="No useful resources in sight, moving forward to discover new tiles.",
            confidence=0.62,
        )


def _normalize_inventory(inventory: Mapping[str, int]) -> dict[str, int]:
    normalized = {name: 0 for name in ["food", *STONE_PRIORITY[::-1]]}
    for name, quantity in inventory.items():
        normalized[str(name)] = int(quantity)
    return normalized


def _normalize_visible_tiles(
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


def _find_current_tile(visible_tiles: Sequence[Mapping[str, object]]) -> Mapping[str, object]:
    for tile in visible_tiles:
        if int(tile["distance"]) == 0:
            return tile
    return visible_tiles[0]

def _decide_incantation_step(
    level: int,
    inventory: Mapping[str, int],
    current_counts: Counter[str],
) -> HeuristicDecision | None:
    requirement = LEVEL_REQUIREMENTS.get(level)
    if requirement is None:
        return None

    needed_on_ground = requirement["stones"]
    for stone, amount in needed_on_ground.items():
        on_ground = current_counts.get(stone, 0)
        if on_ground >= amount:
            continue
        if inventory.get(stone, 0) > 0:
            return HeuristicDecision(
                command=f"Set {stone}",
                rationale=f"need to place {stone} on the tile to prepare the incantation.",
                confidence=0.93,
            )

    players_needed = int(requirement["players"])
    players_here = current_counts.get("player", 0)
    if players_here >= players_needed and _ground_matches_requirement(current_counts, needed_on_ground):
        return HeuristicDecision(
            command="Incantation",
            rationale="the tile appears ready for the incantation.",
            confidence=0.88,
        )

    return None


def _ground_matches_requirement(
    current_counts: Counter[str],
    needed_on_ground: Mapping[str, int],
) -> bool:
    return all(current_counts.get(stone, 0) >= amount for stone, amount in needed_on_ground.items())


def _needed_stones_for_level(level: int, inventory: Mapping[str, int]) -> set[str]:
    requirement = LEVEL_REQUIREMENTS.get(level)
    if requirement is None:
        return set()

    missing = set()
    for stone, amount in requirement["stones"].items():
        if inventory.get(stone, 0) < amount:
            missing.add(stone)
    return missing


def _pick_current_tile_resource(
    current_counts: Counter[str],
    candidates: Sequence[str] | set[str],
) -> str | None:
    for resource in STONE_PRIORITY:
        if resource in candidates and current_counts.get(resource, 0) > 0:
            return resource
    if "food" in candidates and current_counts.get("food", 0) > 0:
        return "food"
    return None


def _select_visible_target(
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


def _move_towards(target: Mapping[str, object], rationale: str) -> HeuristicDecision:
    dx = int(target["x"])
    dy = int(target["y"])
    if dx < 0:
        return HeuristicDecision(
            command="Left",
            rationale=f"{rationale} The best path starts by turning left.",
            confidence=0.76,
        )
    if dx > 0:
        return HeuristicDecision(
            command="Right",
            rationale=f"{rationale} The best path starts by turning right.",
            confidence=0.76,
        )
    if dy > 0:
        return HeuristicDecision(
            command="Forward",
            rationale=f"{rationale} The resource is right in front.",
            confidence=0.8,
        )
    return HeuristicDecision(
        command="Forward",
        rationale=rationale,
        confidence=0.6,
    )
