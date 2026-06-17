"""Simple heuristic model for a Zappy AI client."""

from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
from typing import Mapping, Sequence

try:
    from .utils.model_utils import (
        LEVEL_REQUIREMENTS,
        RESOURCE_NAMES_WITHOUT_FOOD,
        build_plan_to_tile,
        find_current_tile,
        ground_matches_requirement,
        missing_players_for_incantation,
        needed_stones_for_level,
        normalize_inventory,
        normalize_visible_tiles,
        pick_current_tile_resource,
        select_visible_target,
    )
except ImportError:
    from utils.model_utils import (
        LEVEL_REQUIREMENTS,
        RESOURCE_NAMES_WITHOUT_FOOD,
        build_plan_to_tile,
        find_current_tile,
        ground_matches_requirement,
        missing_players_for_incantation,
        needed_stones_for_level,
        normalize_inventory,
        normalize_visible_tiles,
        pick_current_tile_resource,
        select_visible_target,
    )


SURVIVAL_FOOD_THRESHOLD = 5


@dataclass(frozen=True)
class HeuristicDecision:
    command: str
    rationale: str
    confidence: float
    plan: tuple[str, ...] = ()


@dataclass(frozen=True)
class _PredictionContext:
    level: int
    objective: str
    inventory: dict[str, int]
    reserved_resources: dict[str, int]
    visible_tiles: list[dict[str, object]]
    current_counts: Counter[str]
    needed_stones: set[str]
    inventory_ready_for_elevation: bool
    current_tile_preparation: bool


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
        current_tile_reserved_resources: Mapping[str, int] | None = None,
    ) -> HeuristicDecision:
        if not team_name:
            raise ValueError("team_name cannot be empty")

        context = self._build_context(
            level=level,
            objective=objective,
            inventory=inventory,
            visible_tiles=visible_tiles,
            current_tile_reserved_resources=current_tile_reserved_resources,
        )

        # Refresh vision before making any expensive decision.
        if not look_is_fresh or not context.visible_tiles:
            return _build_single_action_decision(
                command="Look",
                rationale="vision not fresh or no visible tiles, need to update vision.",
                confidence=0.99,
            )

        # Keep food above a safe minimum before doing anything risky.
        survival_decision = self._decide_survival(context)
        if survival_decision is not None:
            return survival_decision

        # Handle the next elevation step when inventory or setup says it is time.
        elevation_decision = self._decide_elevation(context)
        if elevation_decision is not None:
            return elevation_decision

        # Outside of elevation, only gather stones that matter for the next level.
        collection_decision = self._decide_general_collection(context)
        if collection_decision is not None:
            return collection_decision

        return self._decide_exploration()

    def _build_context(
        self,
        *,
        level: int,
        objective: str,
        inventory: Mapping[str, int],
        visible_tiles: Sequence[Mapping[str, object]],
        current_tile_reserved_resources: Mapping[str, int] | None,
    ) -> _PredictionContext:
        normalized_inventory = normalize_inventory(inventory)
        normalized_reserved_resources = normalize_inventory(current_tile_reserved_resources or {})
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

        return _PredictionContext(
            level=level,
            objective=objective,
            inventory=normalized_inventory,
            reserved_resources=normalized_reserved_resources,
            visible_tiles=normalized_tiles,
            current_counts=current_counts,
            needed_stones=needed_stones,
            inventory_ready_for_elevation=inventory_ready_for_elevation,
            current_tile_preparation=current_tile_preparation,
        )

    def _decide_survival(self, context: _PredictionContext) -> HeuristicDecision | None:
        if context.inventory["food"] > SURVIVAL_FOOD_THRESHOLD:
            return None

        if context.current_counts["food"] > 0:
            return _build_single_action_decision(
                command="Take food",
                rationale="food is on the current tile, need to take it to survive.",
                confidence=0.98,
            )

        target = select_visible_target(context.visible_tiles, {"food"})
        if target is None:
            return None
        return _move_towards(target, "food is prioritized for survival.")

    def _decide_elevation(self, context: _PredictionContext) -> HeuristicDecision | None:
        ritual_command = _decide_incantation_step(
            context.level,
            context.inventory,
            context.current_counts,
        )
        if ritual_command is not None:
            return ritual_command

        should_prepare_elevation = (
            context.objective == "elevation"
            or context.inventory_ready_for_elevation
            or context.current_tile_preparation
        )
        if not should_prepare_elevation:
            return None

        if context.inventory_ready_for_elevation:
            missing_players = missing_players_for_incantation(
                context.level,
                context.current_counts,
            )
            if missing_players > 0:
                next_level = context.level + 1
                return _build_single_action_decision(
                    command=f"Broadcast incantation niveau {next_level}",
                    rationale=(
                        "inventory ready for the next incantation but not enough players "
                        "are currently on the tile."
                    ),
                    confidence=0.72,
                )

        current_stone = pick_current_tile_resource(
            context.current_counts,
            context.needed_stones,
            reserved_resources=context.reserved_resources,
        )
        if current_stone is not None:
            return _build_single_action_decision(
                command=f"Take {current_stone}",
                rationale=f"current content {current_stone}, can be useful for the incantation.",
                confidence=0.9,
            )

        target = select_visible_target(context.visible_tiles, context.needed_stones)
        if target is None:
            return None
        return _move_towards(target, "stone needed for incantation is visible, moving towards it.")

    def _decide_general_collection(self, context: _PredictionContext) -> HeuristicDecision | None:
        if context.current_counts["food"] > 0:
            return _build_single_action_decision(
                command="Take food",
                rationale="food is on the current tile, need to take it to survive.",
                confidence=0.74,
            )

        current_stone = pick_current_tile_resource(
            context.current_counts,
            context.needed_stones,
        )
        if current_stone is not None:
            return _build_single_action_decision(
                command=f"Take {current_stone}",
                rationale=f"current content {current_stone}, still needed for the next incantation.",
                confidence=0.7,
            )

        target = select_visible_target(context.visible_tiles, context.needed_stones | {"food"})
        if target is None:
            return None
        return _move_towards(target, "A useful resource is visible, moving towards it.")

    def _decide_exploration(self) -> HeuristicDecision:
        return _build_single_action_decision(
            command="Forward",
            rationale="No useful resources in sight, moving forward to discover new tiles.",
            confidence=0.62,
        )


def _build_single_action_decision(
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
            return _build_single_action_decision(
                command=f"Set {stone}",
                rationale=f"need to place {stone} on the tile to prepare the incantation.",
                confidence=0.93,
            )

    players_needed = int(requirement["players"])
    players_here = current_counts.get("player", 0)
    if players_here >= players_needed and ground_matches_requirement(current_counts, needed_on_ground):
        return _build_single_action_decision(
            command="Incantation",
            rationale="the tile appears ready for the incantation.",
            confidence=0.88,
        )

    return None


def _move_towards(target: Mapping[str, object], rationale: str) -> HeuristicDecision:
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
