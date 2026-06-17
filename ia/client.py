"""Client TCP minimal pour tester le modele IA contre ai_lab."""

from __future__ import annotations

import argparse
import socket
import sys
from typing import Any

try:
    from .heuristic_model import HeuristicModel
    from .parsing import LineBuffer, ProtocolError, parse_server_line
except ImportError:
    from heuristic_model import HeuristicModel
    from parsing import LineBuffer, ProtocolError, parse_server_line


RESOURCE_NAMES = (
    "food",
    "linemate",
    "deraumere",
    "sibur",
    "mendiane",
    "phiras",
    "thystame",
)

STONE_NAMES = RESOURCE_NAMES[1:]

INVENTORY_DRIFT_ACTIONS = {
    "Forward",
    "Right",
    "Left",
    "Look",
    "Broadcast",
    "Fork",
    "Eject",
    "Take",
    "Set",
    "Incantation",
}

LOOK_INVALIDATING_ACTIONS = {
    "Forward",
    "Right",
    "Left",
    "Take",
    "Set",
    "Incantation",
    "Eject",
}


class ZappyAIClient:

    def __init__(
        self,
        *,
        host: str,
        port: int,
        team_name: str,
        objective: str = "exploration",
        inventory_refresh_interval: int = 5,
        max_actions: int | None = None,
        verbose: bool = True,
    ) -> None:
        self.host = host
        self.port = port
        self.team_name = team_name
        self.objective = objective
        self.inventory_refresh_interval = max(1, inventory_refresh_interval)
        self.max_actions = max_actions
        self.verbose = verbose

        self.model = HeuristicModel()
        self.socket: socket.socket | None = None
        self.buffer = LineBuffer()
        self.ready_lines: list[str] = []
        self.pending_command: str | None = None
        self.planned_commands: list[str] = []
        self.command_count = 0
        self.dead = False
        self.inventory_is_known = False
        self.actions_since_inventory = self.inventory_refresh_interval
        self.state = {
            "level": 1,
            "team_name": team_name,
            "objective": objective,
            "inventory": {name: 0 for name in RESOURCE_NAMES},
            "current_tile_reserved_resources": {name: 0 for name in STONE_NAMES},
            "visible_tiles": [],
            "look_is_fresh": False,
            "available_slots": None,
            "map_width": None,
            "map_height": None,
            "last_broadcast": None,
            "last_eject_direction": None,
        }

    def connect(self) -> None:
        self.socket = socket.create_connection((self.host, self.port))
        self._log(f"[connect] {self.host}:{self.port}")

        event = self._read_event()
        if event["type"] != "welcome":
            raise RuntimeError(f"Handshake invalide: {event!r}")

        self._send_raw(self.team_name)
        self._log(f"[send] {self.team_name}")

        slots_event = self._read_event()
        if slots_event["type"] == "ko":
            raise RuntimeError(f"Equipe refusee par le serveur: {self.team_name}")
        if slots_event["type"] != "available_slots":
            raise RuntimeError(f"Slots attendus, recu: {slots_event!r}")
        self.state["available_slots"] = int(slots_event["value"])

        size_event = self._read_event()
        if size_event["type"] != "map_size":
            raise RuntimeError(f"Taille de carte attendue, recu: {size_event!r}")
        self.state["map_width"] = int(size_event["width"])
        self.state["map_height"] = int(size_event["height"])

        self._log(
            "[handshake] "
            f"slots={self.state['available_slots']} "
            f"map={self.state['map_width']}x{self.state['map_height']}"
        )

    def close(self) -> None:
        if self.socket is None:
            return
        try:
            self.socket.close()
        finally:
            self.socket = None

    def run(self) -> None:
        self.connect()
        self._send_command("Inventory")

        try:
            while not self.dead:
                event = self._read_event()
                self._handle_event(event)

                if self.pending_command is not None and self._is_terminal_response(
                    self.pending_command,
                    event,
                ):
                    completed_command = self.pending_command
                    self.pending_command = None
                    self._finalize_command(completed_command, event)

                if self.dead:
                    break

                if self.pending_command is None and not self._maybe_send_next_command():
                    break
        finally:
            self.close()

    def _maybe_send_next_command(self) -> bool:
        if self.max_actions is not None and self.command_count >= self.max_actions:
            self._log(f"[stop] limite d'actions atteinte ({self.max_actions})")
            return False

        if self.planned_commands:
            command = self.planned_commands.pop(0)
            self._log(f"[plan] {command} (reste {len(self.planned_commands)})")
            self._send_command(command)
            return True

        if not self.inventory_is_known or self.actions_since_inventory >= self.inventory_refresh_interval:
            self._send_command("Inventory")
            return True

        if not bool(self.state["look_is_fresh"]):
            self._send_command("Look")
            return True

        decision = self.model.predict(
            level=int(self.state["level"]),
            team_name=str(self.state["team_name"]),
            inventory=dict(self.state["inventory"]),
            current_tile_reserved_resources=dict(self.state["current_tile_reserved_resources"]),
            visible_tiles=list(self.state["visible_tiles"]),
            look_is_fresh=bool(self.state["look_is_fresh"]),
            objective=str(self.state["objective"]),
        )
        self._log(
            f"[decision] {decision.command} "
            f"(confiance={decision.confidence:.2f}, raison={decision.rationale})"
        )
        self.planned_commands = list(decision.plan[1:]) if decision.plan else []
        self._send_command(decision.command)
        return True

    def _send_command(self, command: str) -> None:
        self._send_raw(command)
        self.pending_command = command
        self.command_count += 1
        self._log(f"[send] {command}")

    def _send_raw(self, text: str) -> None:
        if self.socket is None:
            raise RuntimeError("Socket non connectee.")
        self.socket.sendall(f"{text}\n".encode("utf-8"))

    def _read_event(self) -> dict[str, Any]:
        while True:
            raw_line = self._read_line()
            if raw_line == "":
                continue

            self._log(f"[recv] {raw_line}")
            try:
                return parse_server_line(raw_line)
            except ProtocolError as exc:
                self._log(f"[warn] ligne non supportee: {exc}")
                return {"type": "raw", "line": raw_line}

    def _read_line(self) -> str:
        if self.socket is None:
            raise RuntimeError("Socket non connectee.")

        while True:
            if self.ready_lines:
                return self.ready_lines.pop(0)

            chunk = self.socket.recv(4096)
            if not chunk:
                raise ConnectionError("Connexion fermee par le serveur.")

            self.ready_lines.extend(self.buffer.feed(chunk))

    def _handle_event(self, event: dict[str, Any]) -> None:
        event_type = str(event["type"])

        if event_type == "inventory":
            self.state["inventory"] = dict(event["resources"])
            self.inventory_is_known = True
            self.actions_since_inventory = 0
            return

        if event_type == "look":
            self.state["visible_tiles"] = build_visible_tiles(event["tiles"])
            self.state["look_is_fresh"] = True
            return

        if event_type == "current_level":
            self.state["level"] = int(event["level"])
            return

        if event_type == "available_slots":
            self.state["available_slots"] = int(event["value"])
            return

        if event_type == "map_size":
            self.state["map_width"] = int(event["width"])
            self.state["map_height"] = int(event["height"])
            return

        if event_type == "broadcast":
            self.state["last_broadcast"] = {
                "direction": int(event["direction"]),
                "message": str(event["message"]),
            }
            return

        if event_type == "eject":
            self.state["last_eject_direction"] = int(event["direction"])
            self.state["look_is_fresh"] = False
            self._clear_current_tile_reserved_resources()
            self.planned_commands.clear()
            return

        if event_type == "dead":
            self.dead = True
            self._clear_current_tile_reserved_resources()
            self.planned_commands.clear()
            self._log("[state] joueur mort")

    def _is_terminal_response(self, command: str, event: dict[str, Any]) -> bool:
        action = command.split()[0]
        event_type = str(event["type"])

        if event_type in {"broadcast", "eject", "raw"}:
            return False

        if action == "Look":
            return event_type in {"look", "dead"}
        if action == "Inventory":
            return event_type in {"inventory", "dead"}
        if action == "Connect_nbr":
            return event_type in {"available_slots", "dead"}
        if action == "Incantation":
            return event_type in {"current_level", "ko", "dead"}
        return event_type in {"ok", "ko", "dead"}

    def _finalize_command(self, command: str, event: dict[str, Any]) -> None:
        action, argument = split_command(command)
        event_type = str(event["type"])

        if action in INVENTORY_DRIFT_ACTIONS and action != "Inventory":
            self.actions_since_inventory += 1

        if event_type == "ko":
            self.planned_commands.clear()

        if action == "Take" and event_type == "ok" and argument is not None:
            self._apply_take(argument)
        elif action == "Set" and event_type == "ok" and argument is not None:
            self._apply_set(argument)
        elif action == "Incantation" and event_type == "current_level":
            self._clear_current_tile_reserved_resources()

        if action in LOOK_INVALIDATING_ACTIONS:
            self.state["look_is_fresh"] = False

        if action == "Forward" and event_type == "ok":
            self._clear_current_tile_reserved_resources()

    def _apply_take(self, resource_name: str) -> None:
        inventory = self.state["inventory"]
        if resource_name in inventory:
            inventory[resource_name] = int(inventory[resource_name]) + 1
        reserved_resources = self.state["current_tile_reserved_resources"]
        if resource_name in reserved_resources and int(reserved_resources[resource_name]) > 0:
            reserved_resources[resource_name] = int(reserved_resources[resource_name]) - 1

    def _apply_set(self, resource_name: str) -> None:
        inventory = self.state["inventory"]
        if resource_name in inventory and int(inventory[resource_name]) > 0:
            inventory[resource_name] = int(inventory[resource_name]) - 1
        reserved_resources = self.state["current_tile_reserved_resources"]
        if resource_name in reserved_resources:
            reserved_resources[resource_name] = int(reserved_resources[resource_name]) + 1

    def _clear_current_tile_reserved_resources(self) -> None:
        self.state["current_tile_reserved_resources"] = {name: 0 for name in STONE_NAMES}

    def _log(self, message: str) -> None:
        if self.verbose:
            print(message)


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


def split_command(command: str) -> tuple[str, str | None]:
    parts = command.split(maxsplit=1)
    if len(parts) == 1:
        return parts[0], None
    return parts[0], parts[1]


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Client IA Zappy minimal pour ai_lab.")
    parser.add_argument("--host", default="127.0.0.1", help="Hote TCP du serveur.")
    parser.add_argument("--port", type=int, default=4242, help="Port TCP du serveur.")
    parser.add_argument("--team", required=True, help="Nom de l'equipe Zappy.")
    parser.add_argument(
        "--objective",
        default="exploration",
        help="Objectif global passe au modele heuristique.",
    )
    parser.add_argument(
        "--inventory-refresh",
        type=int,
        default=5,
        help="Nombre d'actions entre deux Inventory automatiques.",
    )
    parser.add_argument(
        "--max-actions",
        type=int,
        default=None,
        help="Arrete le client apres ce nombre d'actions envoyees.",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Desactive les logs du client.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    client = ZappyAIClient(
        host=args.host,
        port=args.port,
        team_name=args.team,
        objective=args.objective,
        inventory_refresh_interval=args.inventory_refresh,
        max_actions=args.max_actions,
        verbose=not args.quiet,
    )
    try:
        client.run()
    except KeyboardInterrupt:
        print("Arret du client.", file=sys.stderr)
        return 130
    except Exception as exc:
        print(f"Erreur client: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
