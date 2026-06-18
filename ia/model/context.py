"""Context normalization helpers for the heuristic model."""

from __future__ import annotations

from collections import Counter
from typing import Mapping, Sequence

try:
    from ..config import ACTION_COOLDOWN_STEPS
    from ..utils.model_utils import (
        LEVEL_REQUIREMENTS,
        RESOURCE_NAMES_WITHOUT_FOOD,
        find_current_tile,
        needed_stones_for_level,
        normalize_inventory,
        normalize_visible_tiles,
    )
    from .entities import PredictionContext
except ImportError:
    from config import ACTION_COOLDOWN_STEPS
    from model.entities import PredictionContext
    from utils.model_utils import (
        LEVEL_REQUIREMENTS,
        RESOURCE_NAMES_WITHOUT_FOOD,
        find_current_tile,
        needed_stones_for_level,
        normalize_inventory,
        normalize_visible_tiles,
    )


def build_prediction_context(
    *,
    level: int,
    team_name: str,
    objective: str,
    inventory: Mapping[str, int],
    visible_tiles: Sequence[Mapping[str, object]],
    current_tile_reserved_resources: Mapping[str, int] | None,
    available_slots: int | None,
    action_cooldowns: Mapping[str, int] | None,
    safe_turns: int,
    ally_broadcast: Mapping[str, object] | None,
    last_outgoing_broadcast: Mapping[str, object] | None,
) -> PredictionContext:
    normalized_inventory = normalize_inventory(inventory)
    normalized_reserved_resources = normalize_inventory(current_tile_reserved_resources or {})
    normalized_action_cooldowns = normalize_action_cooldowns(action_cooldowns or {})
    normalized_tiles = normalize_visible_tiles(visible_tiles)

    if normalized_tiles:
        current_tile = find_current_tile(normalized_tiles)
        current_counts = Counter(current_tile["items"])
    else:
        current_counts = Counter()

    needed_stones = needed_stones_for_level(level, normalized_inventory)
    inventory_ready_for_elevation = level in LEVEL_REQUIREMENTS and not needed_stones
    current_tile_preparation = any(
        normalized_reserved_resources.get(resource, 0) > 0
        for resource in RESOURCE_NAMES_WITHOUT_FOOD
    )

    return PredictionContext(
        level=level,
        team_name=team_name,
        objective=objective,
        inventory=normalized_inventory,
        reserved_resources=normalized_reserved_resources,
        available_slots=available_slots,
        action_cooldowns=normalized_action_cooldowns,
        safe_turns=max(0, int(safe_turns)),
        ally_broadcast=dict(ally_broadcast) if ally_broadcast is not None else None,
        last_outgoing_broadcast=(
            dict(last_outgoing_broadcast)
            if last_outgoing_broadcast is not None
            else None
        ),
        visible_tiles=normalized_tiles,
        current_counts=current_counts,
        needed_stones=needed_stones,
        inventory_ready_for_elevation=inventory_ready_for_elevation,
        current_tile_preparation=current_tile_preparation,
    )


def normalize_action_cooldowns(action_cooldowns: Mapping[str, int]) -> dict[str, int]:
    normalized = {action_name: 0 for action_name in ACTION_COOLDOWN_STEPS}
    for action_name, value in action_cooldowns.items():
        normalized[str(action_name)] = int(value)
    return normalized
