"""Synchronous TCP client for a single Zappy AI player.

Every command is sent and immediately waited on: this trades the protocol's
theoretical 10-command pipelining for a client that is trivial to follow and
never accumulates stale local state. Round-trips are cheap relative to the
in-game action costs (default f=100), so this is not a real performance
concern in practice.
"""

from __future__ import annotations

import argparse
import os
import sys
from typing import Any

try:
    from .parsing import LineBuffer, ProtocolError, parse_server_line
    from .strategy import Strategy
except ImportError:
    from parsing import LineBuffer, ProtocolError, parse_server_line
    from strategy import Strategy

import socket


GAME_OVER_EXIT_CODE = 20


class ZappyAIClient:

    def __init__(self, *, host: str, port: int, team_name: str, verbose: bool = True) -> None:
        self.host = host
        self.port = port
        self.team_name = team_name
        self.verbose = verbose
        # client.log mixes the stdout of every spawned client process: tag
        # each line so a single client's trace can be grep'd back out.
        self._log_id = f"{team_name}:{os.getpid()}"

        self.socket: socket.socket | None = None
        self.buffer = LineBuffer()
        self.ready_lines: list[str] = []

        self.dead = False
        self.game_over = False
        self.winner_team: str | None = None
        self.available_slots: int | None = None
        self.map_width: int | None = None
        self.map_height: int | None = None
        self.last_food = 0
        # A participant frozen by someone else's incantation gets "Current
        # level: K" pushed unsolicited, with no command of its own pending.
        self.pending_level: int | None = None
        # While frozen for someone else's ritual, our own queued command (if
        # any) is held by the server with no response at all until the
        # ritual resolves. The next ko/current_level we see belongs to that
        # ritual, not to our command - the real response follows afterwards.
        self._frozen_for_ritual = False

        self._pending_messages: list[tuple[int, str]] = []

    # -- connection -----------------------------------------------------------

    def connect(self) -> None:
        self.socket = socket.create_connection((self.host, self.port))
        self._log(f"[connect] {self.host}:{self.port}")

        event = self._read_event()
        if event["type"] == "game_end":
            self.game_over = True
            self.winner_team = str(event["team"])
            self._log(f"[state] fin de partie: {self.winner_team}")
            return
        if event["type"] != "welcome":
            raise RuntimeError(f"Handshake invalide: {event!r}")

        self._send_raw(self.team_name)
        self._log(f"[send] {self.team_name}")

        slots_event = self._read_event()
        if slots_event["type"] == "ko":
            raise RuntimeError(f"Equipe refusee par le serveur: {self.team_name}")
        if slots_event["type"] != "available_slots":
            raise RuntimeError(f"Slots attendus, recu: {slots_event!r}")
        self.available_slots = int(slots_event["value"])

        size_event = self._read_event()
        if size_event["type"] != "map_size":
            raise RuntimeError(f"Taille de carte attendue, recu: {size_event!r}")
        self.map_width = int(size_event["width"])
        self.map_height = int(size_event["height"])

        self._log(
            f"[handshake] slots={self.available_slots} map={self.map_width}x{self.map_height}"
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
        if self.game_over:
            return

        strategy = Strategy(self, self.team_name)
        try:
            while not self.dead and not self.game_over:
                if not strategy.step():
                    break
        finally:
            self.close()

        if self.dead:
            print(f"dead level={strategy.level} food={self.last_food}", file=sys.stderr)

    # -- protocol commands used by the strategy --------------------------------

    def forward(self) -> str:
        return self._send_and_wait("Forward", {"ok", "ko"})["type"]

    def left(self) -> str:
        return self._send_and_wait("Left", {"ok", "ko"})["type"]

    def right(self) -> str:
        return self._send_and_wait("Right", {"ok", "ko"})["type"]

    def take(self, item: str) -> str:
        return self._send_and_wait(f"Take {item}", {"ok", "ko"})["type"]

    def set_obj(self, item: str) -> str:
        return self._send_and_wait(f"Set {item}", {"ok", "ko"})["type"]

    def fork(self) -> str:
        return self._send_and_wait("Fork", {"ok", "ko"})["type"]

    def connect_nbr(self) -> int:
        event = self._send_and_wait("Connect_nbr", {"available_slots"})
        if event["type"] != "available_slots":
            return 0
        return int(event["value"])

    def eject(self) -> str:
        return self._send_and_wait("Eject", {"ok", "ko"})["type"]

    def broadcast(self, text: str) -> str:
        return self._send_and_wait(f"Broadcast {text}", {"ok", "ko"})["type"]

    def look(self) -> list[list[str]]:
        event = self._send_and_wait("Look", {"look"})
        if event["type"] != "look":
            return []
        return list(event["tiles"])

    def inventory(self) -> dict[str, int]:
        event = self._send_and_wait("Inventory", {"inventory"})
        if event["type"] != "inventory":
            return {}
        resources = dict(event["resources"])
        self.last_food = int(resources.get("food", 0))
        return resources

    def incantation(self) -> int | None:
        event = self._send_and_wait("Incantation", {"elevation_underway", "ko"})
        if event["type"] != "elevation_underway":
            return None
        event = self._wait_for({"current_level", "ko"})
        if event["type"] != "current_level":
            return None
        return int(event["level"])

    def pop_messages(self) -> list[tuple[int, str]]:
        messages = self._pending_messages
        self._pending_messages = []
        return messages

    # -- low-level I/O ----------------------------------------------------------

    def _send_and_wait(self, command: str, terminal_types: set[str]) -> dict[str, Any]:
        if self.dead or self.game_over:
            return {"type": "dead" if self.dead else "game_end"}
        self._send_raw(command)
        self._log(f"[send] {command}")
        return self._wait_for(terminal_types)

    def _wait_for(self, terminal_types: set[str]) -> dict[str, Any]:
        if self.dead or self.game_over:
            return {"type": "dead" if self.dead else "game_end"}

        while True:
            event = self._read_event()
            event_type = str(event["type"])

            if event_type == "broadcast":
                self._pending_messages.append((int(event["direction"]), str(event["message"])))
                continue
            if event_type == "eject":
                continue
            if event_type == "dead":
                self.dead = True
                return event
            if event_type == "game_end":
                self.game_over = True
                self.winner_team = str(event["team"])
                return event
            if event_type == "elevation_underway":
                if "elevation_underway" in terminal_types:
                    return event
                # Frozen by someone else's ritual: our own queued command (if
                # any) gets no response at all until it resolves.
                self._frozen_for_ritual = True
                continue
            if self._frozen_for_ritual and event_type in ("ko", "current_level"):
                # This is the ritual's resolution, not our command's answer
                # - the real one (if any) still follows once unfrozen.
                if event_type == "current_level":
                    self.pending_level = int(event["level"])
                self._frozen_for_ritual = False
                continue
            if event_type == "current_level":
                # Pushed to every participant of a successful incantation,
                # not just whoever sent the Incantation command.
                self.pending_level = int(event["level"])
                if "current_level" in terminal_types:
                    return event
                continue
            if event_type in terminal_types:
                return event

            self._log(f"[warn] evenement inattendu ignore: {event}")

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

    def _log(self, message: str) -> None:
        if self.verbose:
            print(f"[{self._log_id}] {message}")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Client IA Zappy.")
    parser.add_argument("--host", default="127.0.0.1", help="Hote TCP du serveur.")
    parser.add_argument("--port", type=int, default=4242, help="Port TCP du serveur.")
    parser.add_argument("--team", required=True, help="Nom de l'equipe Zappy.")
    parser.add_argument("--quiet", action="store_true", help="Desactive les logs du client.")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    client = ZappyAIClient(host=args.host, port=args.port, team_name=args.team, verbose=not args.quiet)
    try:
        client.run()
    except KeyboardInterrupt:
        print("Arret du client.", file=sys.stderr)
        return 130
    except Exception as exc:
        print(f"Erreur client: {exc}", file=sys.stderr)
        return 1
    if client.game_over:
        return GAME_OVER_EXIT_CODE
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
