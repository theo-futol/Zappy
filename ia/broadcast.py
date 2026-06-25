"""Rally broadcast format and sound-direction helpers.

The AI only ever broadcasts one kind of message: a "ready" ping advertising
that the sender has decent food and is carrying some of the stones needed
for its current level transition. The stone list lets receivers predict
whether converging would actually pool enough resources before they commit
to the trip.
Format: "level|team|token|stone:qty+stone:qty" (or "none" with no stones).
"""

from __future__ import annotations

from typing import Mapping

_RESOURCE_SEPARATOR = "+"
_QUANTITY_SEPARATOR = ":"
_NONE_TOKEN = "none"


def build_rally_message(*, level: int, team: str, token: str, resources: Mapping[str, int]) -> str:
    return f"{int(level)}|{team}|{token}|{_format_resources(resources)}"


def parse_rally_message(text: str) -> tuple[int, str, str, dict[str, int]] | None:
    parts = text.strip().split("|", maxsplit=3)
    if len(parts) != 4:
        return None
    level_text, team, token, resource_text = parts
    if not level_text.isdigit() or not team or not token:
        return None
    resources = _parse_resources(resource_text)
    if resources is None:
        return None
    return int(level_text), team, token, resources


def _format_resources(resources: Mapping[str, int]) -> str:
    parts = [f"{name}{_QUANTITY_SEPARATOR}{int(qty)}" for name, qty in resources.items() if qty > 0]
    return _RESOURCE_SEPARATOR.join(parts) if parts else _NONE_TOKEN


def _parse_resources(text: str) -> dict[str, int] | None:
    normalized = text.strip().lower()
    if not normalized or normalized == _NONE_TOKEN:
        return {}
    resources: dict[str, int] = {}
    for item in normalized.split(_RESOURCE_SEPARATOR):
        if _QUANTITY_SEPARATOR not in item:
            return None
        name, _, qty_text = item.partition(_QUANTITY_SEPARATOR)
        if not name or not qty_text.isdigit():
            return None
        resources[name] = int(qty_text)
    return resources


def build_plan_from_sound_direction(direction: int) -> tuple[str, ...]:
    plans = {
        0: (),
        1: ("Forward",),
        2: ("Left", "Forward"),
        3: ("Left", "Forward"),
        4: ("Left", "Left", "Forward"),
        5: ("Left", "Left", "Forward"),
        6: ("Right", "Right", "Forward"),
        7: ("Right", "Forward"),
        8: ("Right", "Forward"),
    }
    return plans.get(int(direction), ())
