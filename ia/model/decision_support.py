"""Pure decision helpers shared by the state machine."""

from __future__ import annotations

from collections import Counter
from typing import Mapping

try:
    from ..config import INCANTATION_FOOD_THRESHOLD
    from ..utils.model_utils import (
        LEVEL_REQUIREMENTS,
        build_plan_to_tile,
        ground_matches_requirement,
    )
    from .entities import HeuristicDecision
except ImportError:
    from config import INCANTATION_FOOD_THRESHOLD
    from model.entities import HeuristicDecision
    from utils.model_utils import (
        LEVEL_REQUIREMENTS,
        build_plan_to_tile,
        ground_matches_requirement,
    )


def build_single_action_decision(
    *,
    command: str,
    rationale: str,
    confidence: float,
) -> HeuristicDecision:
    return HeuristicDecision(
        command=command,
        rationale=rationale,
        confidence=confidence,
        plan=(command,),
    )


def tile_is_ready_for_incantation(
    level: int,
    current_counts: Counter[str],
) -> bool:
    requirement = LEVEL_REQUIREMENTS.get(level)
    if requirement is None:
        return False
    return ground_matches_requirement(current_counts, requirement["stones"])


def has_enough_food_for_incantation(inventory: Mapping[str, int]) -> bool:
    return int(inventory.get("food", 0)) >= INCANTATION_FOOD_THRESHOLD


def decide_incantation_step(
    level: int,
    inventory: Mapping[str, int],
    current_counts: Counter[str],
) -> HeuristicDecision | None:
    requirement = LEVEL_REQUIREMENTS.get(level)
    if requirement is None:
        return None
    if not has_enough_food_for_incantation(inventory):
        return None

    needed_on_ground = requirement["stones"]
    for stone, amount in needed_on_ground.items():
        on_ground = current_counts.get(stone, 0)
        if on_ground >= amount:
            continue
        if inventory.get(stone, 0) > 0:
            return build_single_action_decision(
                command=f"Set {stone}",
                rationale=f"need to place {stone} on the tile to prepare the incantation.",
                confidence=0.93,
            )

    players_needed = int(requirement["players"])
    players_here = current_counts.get("player", 0)
    if players_here >= players_needed and ground_matches_requirement(current_counts, needed_on_ground):
        return build_single_action_decision(
            command="Incantation",
            rationale="the tile appears ready for the incantation.",
            confidence=0.88,
        )

    return None


def move_towards(target: Mapping[str, object], rationale: str) -> HeuristicDecision:
    plan = build_plan_to_tile(target)
    command = plan[0]
    if len(plan) == 1:
        return HeuristicDecision(
            command=command,
            rationale=f"{rationale} The target is directly ahead.",
            confidence=0.8,
            plan=plan,
        )

    return HeuristicDecision(
        command=command,
        rationale=f"{rationale} Following a short plan to reach the visible tile: {' -> '.join(plan)}.",
        confidence=0.82,
        plan=plan,
    )
