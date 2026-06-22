"""Helpers for structured Zappy broadcast messages."""

from __future__ import annotations

from typing import Mapping

try:
    from .utils.model_utils import LEVEL_REQUIREMENTS
except ImportError:
    from utils.model_utils import LEVEL_REQUIREMENTS


BROADCAST_NONE_TOKEN = "none"
BROADCAST_INTENTION_INCANTATION = "incantation_call"
BROADCAST_INTENTION_INCANTATION_AVAILABLE = "incantation_available"
BROADCAST_INTENTION_INCANTATION_ARRIVING = "incantation_arriving"
BROADCAST_INTENTION_EXPLORATION = "exploration"

INCANTATION_CALL_INTENTIONS = {
    "incantation",
    BROADCAST_INTENTION_INCANTATION,
}

INCANTATION_SUPPORT_INTENTIONS = {
    BROADCAST_INTENTION_INCANTATION_AVAILABLE,
    BROADCAST_INTENTION_INCANTATION_ARRIVING,
}

INCANTATION_INTENTIONS = INCANTATION_CALL_INTENTIONS | INCANTATION_SUPPORT_INTENTIONS


def build_broadcast_message(
    *,
    level: int,
    intention: str,
    leader_token: str | None = None,
    resources: Mapping[str, int] | None = None,
) -> str:
    resource_text = format_broadcast_resources(resources or {})
    normalized_intention = normalize_broadcast_intention(intention)
    normalized_leader_token = normalize_broadcast_leader_token(leader_token)
    if is_incantation_broadcast_intention(normalized_intention):
        return (
            f"{int(level)}, {normalized_intention}, "
            f"{normalized_leader_token}, {resource_text}"
        )
    return f"{int(level)}, {normalized_intention}, {resource_text}"


def parse_broadcast_message(message: str) -> dict[str, object] | None:
    raw_message = message.strip()
    if not raw_message:
        return None

    parts = [part.strip() for part in raw_message.split(",", maxsplit=3)]
    if len(parts) not in {3, 4}:
        return None

    if len(parts) == 3:
        level_text, intention_text, resource_text = parts
        leader_token = None
    else:
        level_text, intention_text, leader_token_text, resource_text = parts
        leader_token = normalize_broadcast_leader_token(leader_token_text)

    if not level_text.isdigit():
        return None

    try:
        resources = parse_broadcast_resources(resource_text)
    except ValueError:
        return None

    return {
        "level": int(level_text),
        "intention": normalize_broadcast_intention(intention_text),
        "leader_token": leader_token,
        "resources": resources,
    }


def format_broadcast_resources(resources: Mapping[str, int]) -> str:
    parts: list[str] = []
    for resource_name, quantity in resources.items():
        normalized_quantity = int(quantity)
        if normalized_quantity <= 0:
            continue
        parts.append(f"{normalized_quantity} {resource_name}")

    if not parts:
        return BROADCAST_NONE_TOKEN
    return " ".join(parts)


def parse_broadcast_resources(resource_text: str) -> dict[str, int]:
    normalized_text = resource_text.strip()
    if not normalized_text or normalized_text.lower() == BROADCAST_NONE_TOKEN:
        return {}

    tokens = normalized_text.split()
    if len(tokens) % 2 != 0:
        raise ValueError(f"Invalid broadcast resources: {resource_text!r}")

    resources: dict[str, int] = {}
    for index in range(0, len(tokens), 2):
        quantity_text = tokens[index]
        resource_name = tokens[index + 1]
        if not quantity_text.isdigit():
            raise ValueError(f"Invalid broadcast quantity: {quantity_text!r}")
        resources[resource_name] = int(quantity_text)
    return resources


def build_missing_incantation_resources(
    *,
    level: int,
    inventory: Mapping[str, int],
) -> dict[str, int]:
    requirement = LEVEL_REQUIREMENTS.get(level)
    if requirement is None:
        return {}

    missing_resources: dict[str, int] = {}
    required_stones = requirement["stones"]
    for resource_name, required_quantity in required_stones.items():
        current_quantity = int(inventory.get(resource_name, 0))
        missing_quantity = int(required_quantity) - current_quantity
        if missing_quantity > 0:
            missing_resources[resource_name] = missing_quantity
    return missing_resources


def infer_broadcast_intention(command_argument: str | None, objective: str) -> str:
    normalized_argument = (command_argument or "").strip().lower()
    if "incant" in normalized_argument:
        return BROADCAST_INTENTION_INCANTATION
    if normalized_argument:
        return normalize_broadcast_intention(normalized_argument)
    if objective:
        return normalize_broadcast_intention(objective)
    return BROADCAST_INTENTION_EXPLORATION


def normalize_broadcast_intention(intention: str) -> str:
    normalized_intention = intention.strip().lower().replace(" ", "_")
    if not normalized_intention:
        return BROADCAST_INTENTION_EXPLORATION
    if normalized_intention == "incantation":
        return BROADCAST_INTENTION_INCANTATION
    return normalized_intention


def normalize_broadcast_leader_token(leader_token: str | None) -> str | None:
    normalized_leader_token = (leader_token or "").strip().lower()
    if not normalized_leader_token or normalized_leader_token == BROADCAST_NONE_TOKEN:
        return None
    return normalized_leader_token


def is_incantation_broadcast_intention(intention: str) -> bool:
    return normalize_broadcast_intention(intention) in INCANTATION_INTENTIONS


def is_incantation_call_intention(intention: str) -> bool:
    return normalize_broadcast_intention(intention) in INCANTATION_CALL_INTENTIONS


def is_incantation_support_intention(intention: str) -> bool:
    return normalize_broadcast_intention(intention) in INCANTATION_SUPPORT_INTENTIONS


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
