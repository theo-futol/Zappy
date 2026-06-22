#!/usr/bin/env python3
"""Petit runner manuel du modele heuristique."""

try:
    from heuristic_model import HeuristicModel
except ModuleNotFoundError:
    from heuristic_model import HeuristicModel


def build_demo_observations() -> list[dict[str, object]]:
    return [
        {
            "level": 1,
            "team_name": "alpha",
            "look_is_fresh": False,
            "objective": "survie",
            "inventory": {
                "food": 3,
                "linemate": 0,
                "deraumere": 0,
                "sibur": 0,
                "mendiane": 0,
                "phiras": 0,
                "thystame": 0,
            },
            "visible_tiles": build_visible_tiles([["player"], [], ["food"], ["linemate"]]),
        },
        {
            "level": 1,
            "team_name": "alpha",
            "look_is_fresh": True,
            "objective": "elevation",
            "inventory": {
                "food": 9,
                "linemate": 0,
                "deraumere": 0,
                "sibur": 0,
                "mendiane": 0,
                "phiras": 0,
                "thystame": 0,
            },
            "visible_tiles": build_visible_tiles([["player"], ["linemate"], [], []]),
        },
        {
            "level": 2,
            "team_name": "alpha",
            "look_is_fresh": True,
            "objective": "elevation",
            "inventory": {
                "food": 8,
                "linemate": 1,
                "deraumere": 1,
                "sibur": 1,
                "mendiane": 0,
                "phiras": 0,
                "thystame": 0,
            },
            "visible_tiles": build_visible_tiles([["player", "player"], [], [], []]),
        },
        {
            "level": 3,
            "team_name": "alpha",
            "look_is_fresh": True,
            "objective": "exploration",
            "inventory": {
                "food": 12,
                "linemate": 0,
                "deraumere": 0,
                "sibur": 0,
                "mendiane": 0,
                "phiras": 0,
                "thystame": 0,
            },
            "visible_tiles": build_visible_tiles([["player"], [], [], []]),
        },
    ]


def build_visible_tiles(look_tiles: list[list[str]]) -> list[dict[str, object]]:
    visible_tiles = []
    for index, (x, y) in enumerate(tile_positions(len(look_tiles))):
        visible_tiles.append(
            {
                "index": index,
                "x": x,
                "y": y,
                "distance": abs(x) + y,
                "items": look_tiles[index],
            }
        )
    return visible_tiles


def tile_positions(tile_count: int) -> list[tuple[int, int]]:
    positions = [(0, 0)]
    distance = 1
    while len(positions) < tile_count:
        for x in range(-distance, distance + 1):
            positions.append((x, distance))
            if len(positions) == tile_count:
                break
        distance += 1
    return positions


def describe_observation(observation: dict[str, object]) -> str:
    level = int(observation.get("level", 1))
    team_name = str(observation.get("team_name", ""))
    look_is_fresh = bool(observation.get("look_is_fresh", False))
    objective = str(observation.get("objective", "exploration"))
    inventory = observation.get("inventory", {})
    visible_tiles = observation.get("visible_tiles", [])

    if not isinstance(inventory, dict):
        raise TypeError("'inventory' doit etre un dictionnaire.")
    if not isinstance(visible_tiles, list):
        raise TypeError("'visible_tiles' doit etre une liste.")

    stones = ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]
    inventory_text = ", ".join(f"{stone}={int(inventory.get(stone, 0))}" for stone in stones)

    visible = []
    for tile in visible_tiles:
        items = tile.get("items", [])
        if items:
            visible.append(
                f"{int(tile.get('index', 0))}@({int(tile.get('x', 0))},{int(tile.get('y', 0))}):"
                f"{' '.join(str(token) for token in items)}"
            )
    visible_text = ", ".join(visible) if visible else "aucune case visible"

    return (
        f"team {team_name}, niveau {level}, objectif {objective}, look_fresh {look_is_fresh}, "
        f"nourriture {int(inventory.get('food', 0))}, "
        f"inventaire pierres {inventory_text}, vision {visible_text}"
    )


def main() -> None:
    print("Modele heuristique pret.")
    print("Modele : FSM deterministe avec transitions simples")
    print()
    
    for observation in build_demo_observations():
        model = HeuristicModel()
        decision = model.predict(**observation)
        print("Observation :", describe_observation(observation))
        print("Etat :", decision.state)
        print("Commande proposee :", decision.command)
        print("Confiance :", f"{decision.confidence:.2f}")
        print("Justification :", decision.rationale)
        print("-" * 80)


if __name__ == "__main__":
    main()
