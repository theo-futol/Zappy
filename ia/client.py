"""Client TCP minimal pour tester le modele IA contre ai_lab."""

from __future__ import annotations

import argparse
import socket
import sys
from typing import Any

try:
    from .broadcast import (
        BROADCAST_INTENTION_INCANTATION,
        build_broadcast_message,
        build_missing_incantation_resources,
        infer_broadcast_intention,
        parse_broadcast_message,
    )
    from .config import (
        ACTION_COOLDOWN_STEPS,
        DEFAULT_INVENTORY_REFRESH_INTERVAL,
        DEFAULT_OBJECTIVE,
        LOOK_REFRESH_INTERVAL,
        ALLY_HELP_FOOD_THRESHOLD,
        OPPORTUNISTIC_FOOD_THRESHOLD,
        get_ally_broadcast_max_age,
    )
    from .heuristic_model import HeuristicModel
    from .parsing import LineBuffer, ProtocolError, parse_server_line
except ImportError:
    from broadcast import (
        BROADCAST_INTENTION_INCANTATION,
        build_broadcast_message,
        build_missing_incantation_resources,
        infer_broadcast_intention,
        parse_broadcast_message,
    )
    from config import (
        ACTION_COOLDOWN_STEPS,
        DEFAULT_INVENTORY_REFRESH_INTERVAL,
        DEFAULT_OBJECTIVE,
        LOOK_REFRESH_INTERVAL,
        ALLY_HELP_FOOD_THRESHOLD,
        OPPORTUNISTIC_FOOD_THRESHOLD,
        get_ally_broadcast_max_age,
    )
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

LOOK_REFRESH_ON_KO_ACTIONS = {
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
        objective: str = DEFAULT_OBJECTIVE,
        inventory_refresh_interval: int = DEFAULT_INVENTORY_REFRESH_INTERVAL,
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
            "action_cooldowns": {name: 0 for name in ACTION_COOLDOWN_STEPS},
            "safe_turns": 0,
            "actions_since_look": 0,
            "visible_tiles": [],
            "look_is_fresh": False,
            "available_slots": None,
            "map_width": None,
            "map_height": None,
            "ally_broadcast": None,
            "last_broadcast": None,
            "last_outgoing_broadcast": None,
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
            available_slots=self.state["available_slots"],
            action_cooldowns=dict(self.state["action_cooldowns"]),
            safe_turns=int(self.state["safe_turns"]),
            ally_broadcast=self.state["ally_broadcast"],
            last_outgoing_broadcast=self.state["last_outgoing_broadcast"],
            visible_tiles=list(self.state["visible_tiles"]),
            look_is_fresh=bool(self.state["look_is_fresh"]),
            objective=str(self.state["objective"]),
        )
        self._log(
            f"[decision] {decision.state or 'unknown'} -> {decision.command} "
            f"(confiance={decision.confidence:.2f}, raison={decision.rationale})"
        )
        self.planned_commands = list(decision.plan[1:]) if decision.plan else []
        self._send_command(decision.command)
        return True

    def _send_command(self, command: str) -> None:
        prepared_command = self._prepare_command(command)
        self._send_raw(prepared_command)
        self.pending_command = prepared_command
        self.command_count += 1
        self._log(f"[send] {prepared_command}")

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
            self.state["actions_since_look"] = 0
            return

        if event_type == "current_level":
            self.state["level"] = int(event["level"])
            self.state["ally_broadcast"] = None
            self.state["last_outgoing_broadcast"] = None
            return

        if event_type == "available_slots":
            self.state["available_slots"] = int(event["value"])
            return

        if event_type == "map_size":
            self.state["map_width"] = int(event["width"])
            self.state["map_height"] = int(event["height"])
            return

        if event_type == "broadcast":
            parsed_message = parse_broadcast_message(str(event["message"]))
            self.state["last_broadcast"] = {
                "direction": int(event["direction"]),
                "message": str(event["message"]),
                "payload": parsed_message,
            }
            self._update_ally_broadcast(int(event["direction"]), parsed_message)
            return

        if event_type == "eject":
            self.state["last_eject_direction"] = int(event["direction"])
            self.state["look_is_fresh"] = False
            self.state["actions_since_look"] = LOOK_REFRESH_INTERVAL
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

        self._tick_action_cooldowns()

        if action == "Take" and event_type == "ok" and argument is not None:
            self._apply_take(argument)
        elif action == "Set" and event_type == "ok" and argument is not None:
            self._apply_set(argument)
        elif action == "Incantation" and event_type == "current_level":
            self._clear_current_tile_reserved_resources()
        elif action == "Fork" and event_type == "ok":
            self._apply_fork()

        self._set_action_cooldown(action)

        if action in LOOK_INVALIDATING_ACTIONS and event_type in {"ok", "current_level"}:
            self._mark_vision_changed()
        elif action in LOOK_REFRESH_ON_KO_ACTIONS and event_type == "ko":
            self.state["look_is_fresh"] = False
            self.state["actions_since_look"] = LOOK_REFRESH_INTERVAL

        if action == "Forward" and event_type == "ok":
            self._clear_current_tile_reserved_resources()

        self._update_safe_turns(action, event_type)
        self._age_ally_broadcast()
        self._age_last_outgoing_broadcast()

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

    def _apply_fork(self) -> None:
        if self.state["available_slots"] is not None:
            self.state["available_slots"] = int(self.state["available_slots"]) + 1

    def _mark_vision_changed(self) -> None:
        self.state["actions_since_look"] = int(self.state["actions_since_look"]) + 1
        if int(self.state["actions_since_look"]) >= LOOK_REFRESH_INTERVAL:
            self.state["look_is_fresh"] = False

    def _update_safe_turns(self, action: str, event_type: str) -> None:
        if action in {"Inventory", "Look"} or event_type == "dead":
            return

        if event_type == "ok" and action in {"Fork", "Eject"}:
            self.state["safe_turns"] = 0
            return

        if int(self.state["inventory"].get("food", 0)) < OPPORTUNISTIC_FOOD_THRESHOLD:
            self.state["safe_turns"] = 0
            return

        self.state["safe_turns"] = int(self.state["safe_turns"]) + 1

    def _update_ally_broadcast(
        self,
        direction: int,
        payload: dict[str, object] | None,
    ) -> None:
        if not self._is_ally_incantation_broadcast(payload):
            return

        self.state["ally_broadcast"] = {
            "direction": int(direction),
            "payload": dict(payload),
            "age": 0,
        }
        self.planned_commands.clear()

    def _is_ally_incantation_broadcast(
        self,
        payload: dict[str, object] | None,
    ) -> bool:
        if payload is None:
            return False
        if payload.get("intention") != BROADCAST_INTENTION_INCANTATION:
            return False
        if int(payload.get("level", 0)) != int(self.state["level"]):
            return False
        if int(self.state["inventory"].get("food", 0)) < ALLY_HELP_FOOD_THRESHOLD:
            return False
        return True

    def _age_ally_broadcast(self) -> None:
        ally_broadcast = self.state["ally_broadcast"]
        if ally_broadcast is None:
            return

        payload = ally_broadcast.get("payload")
        broadcast_level = int(self.state["level"])
        if isinstance(payload, dict):
            broadcast_level = int(payload.get("level", broadcast_level))

        next_age = int(ally_broadcast.get("age", 0)) + 1
        if next_age > get_ally_broadcast_max_age(broadcast_level):
            self.state["ally_broadcast"] = None
            return

        ally_broadcast["age"] = next_age

    def _age_last_outgoing_broadcast(self) -> None:
        last_outgoing_broadcast = self.state["last_outgoing_broadcast"]
        if last_outgoing_broadcast is None:
            return
        last_outgoing_broadcast["age"] = int(last_outgoing_broadcast.get("age", 0)) + 1

    def _prepare_command(self, command: str) -> str:
        action, argument = split_command(command)
        if action != "Broadcast":
            return command

        if argument is not None:
            structured_payload = parse_broadcast_message(argument)
            if structured_payload is not None:
                self.state["last_outgoing_broadcast"] = {
                    **dict(structured_payload),
                    "age": 0,
                }
                return command

        intention = infer_broadcast_intention(argument, str(self.state["objective"]))
        resources = self._build_broadcast_resources(intention)
        payload = build_broadcast_message(
            level=int(self.state["level"]),
            intention=intention,
            resources=resources,
        )
        self.state["last_outgoing_broadcast"] = {
            "level": int(self.state["level"]),
            "intention": intention,
            "resources": dict(resources),
            "age": 0,
        }
        return f"Broadcast {payload}"

    def _build_broadcast_resources(self, intention: str) -> dict[str, int]:
        if intention == "incantation":
            return build_missing_incantation_resources(
                level=int(self.state["level"]),
                inventory=dict(self.state["inventory"]),
            )
        return {}

    def _tick_action_cooldowns(self) -> None:
        cooldowns = self.state["action_cooldowns"]
        for action_name, value in list(cooldowns.items()):
            cooldowns[action_name] = max(0, int(value) - 1)

    def _set_action_cooldown(self, action_name: str) -> None:
        cooldown_steps = ACTION_COOLDOWN_STEPS.get(action_name)
        if cooldown_steps is None:
            return
        self.state["action_cooldowns"][action_name] = cooldown_steps

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
        default=DEFAULT_OBJECTIVE,
        help="Objectif global passe au modele heuristique.",
    )
    parser.add_argument(
        "--inventory-refresh",
        type=int,
        default=DEFAULT_INVENTORY_REFRESH_INTERVAL,
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
