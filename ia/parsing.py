"""Parseurs du protocole"""

from __future__ import annotations

import re


RESOURCE_NAMES = (
    "food",
    "linemate",
    "deraumere",
    "sibur",
    "mendiane",
    "phiras",
    "thystame",
)

RESOURCE_IDS = set(RESOURCE_NAMES)

_BROADCAST_RE = re.compile(r"^message\s+(\d+),\s?(.*)$")
_EJECT_RE = re.compile(r"^eject:\s*(\d+)$")
_LEVEL_RE = re.compile(r"^Current level:\s+(\d+)$")


class ProtocolError(ValueError):
    """Erreur de parsing du protocole Zappy."""


class LineBuffer:

    def __init__(self) -> None:
        self._buffer = ""

    def feed(self, chunk: str | bytes) -> list[str]:
        text = chunk.decode("utf-8") if isinstance(chunk, bytes) else chunk
        self._buffer += text
        lines = self._buffer.split("\n")
        self._buffer = lines.pop()
        return [line.rstrip("\r") for line in lines]

    @property
    def pending(self) -> str:
        return self._buffer


def parse_server_line(line: str) -> dict[str, object]:
    raw = _normalize_line(line)

    if raw == "WELCOME":
        return {"type": "welcome"}
    if raw == "ok":
        return {"type": "ok"}
    if raw == "ko":
        return {"type": "ko"}
    if raw == "dead":
        return {"type": "dead"}
    if raw == "Elevation underway":
        return {"type": "elevation_underway"}
    if raw.isdigit():
        return {"type": "available_slots", "value": int(raw)}

    level_match = _LEVEL_RE.match(raw)
    if level_match is not None:
        return {"type": "current_level", "level": int(level_match.group(1))}

    broadcast_match = _BROADCAST_RE.match(raw)
    if broadcast_match is not None:
        return {
            "type": "broadcast",
            "direction": int(broadcast_match.group(1)),
            "message": broadcast_match.group(2),
        }

    eject_match = _EJECT_RE.match(raw)
    if eject_match is not None:
        return {
            "type": "eject",
            "direction": int(eject_match.group(1)),
        }

    if raw.startswith("[") and raw.endswith("]"):
        if _looks_like_inventory_payload(raw):
            return {
                "type": "inventory",
                "resources": parse_inventory_payload(raw),
            }
        return {
            "type": "look",
            "tiles": parse_look_payload(raw),
        }

    tokens = raw.split()
    if len(tokens) == 2 and all(token.lstrip("-").isdigit() for token in tokens):
        return {
            "type": "map_size",
            "width": int(tokens[0]),
            "height": int(tokens[1]),
        }

    raise ProtocolError(f"Unsupported AI server line: {raw!r}")


def parse_look_payload(payload: str) -> list[list[str]]:
    inner = _strip_brackets(payload)
    if inner == "":
        return []

    tiles = []
    for tile in inner.split(","):
        tiles.append([part for part in tile.strip().split() if part])
    return tiles


def parse_inventory_payload(payload: str) -> dict[str, int]:
    inner = _strip_brackets(payload)
    resources = {name: 0 for name in RESOURCE_NAMES}
    if inner == "":
        return resources

    for entry in inner.split(","):
        item = entry.strip()
        if not item:
            continue
        parts = item.split()
        if len(parts) != 2:
            raise ProtocolError(f"Invalid inventory entry: {item!r}")
        name, value = parts
        _validate_resource_name(name)
        resources[name] = int(value)
    return resources


def _normalize_line(line: str) -> str:
    return line.rstrip("\r\n")


def _strip_brackets(payload: str) -> str:
    raw = _normalize_line(payload)
    if not (raw.startswith("[") and raw.endswith("]")):
        raise ProtocolError(f"Invalid bracket payload: {payload!r}")
    return raw[1:-1]


def _looks_like_inventory_payload(payload: str) -> bool:
    inner = _strip_brackets(payload)
    if inner == "":
        return True

    for entry in inner.split(","):
        item = entry.strip()
        if not item:
            continue
        parts = item.split()
        if len(parts) != 2:
            return False
        name, value = parts
        if name not in RESOURCE_IDS or not value.lstrip("-").isdigit():
            return False
    return True


def _validate_resource_name(resource_name: str) -> None:
    if resource_name not in RESOURCE_IDS:
        raise ProtocolError(f"Unknown resource name: {resource_name!r}")
