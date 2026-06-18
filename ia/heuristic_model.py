"""Deterministic finite-state model for a Zappy AI client."""

from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
from enum import Enum
from typing import Mapping, Sequence

try:
    from .broadcast import BROADCAST_INTENTION_INCANTATION, build_plan_from_sound_direction
    from .config import (
        ACTION_COOLDOWN_STEPS,
        ALLY_HELP_FOOD_THRESHOLD,
        DEFAULT_OBJECTIVE,
        EXPLORE_TURN_INTERVAL,
        FORCE_OPPORTUNISTIC_AFTER_SAFE_TURNS,
        FORK_FOOD_THRESHOLD,
        FORK_SAFE_TURNS_THRESHOLD,
        INCANTATION_FOOD_THRESHOLD,
        OPPORTUNISTIC_FOOD_THRESHOLD,
        OVERCROWD_EJECT_THRESHOLD,
        PLACE_STONES_ONLY_WHEN_INVENTORY_READY,
        SURVIVAL_FOOD_THRESHOLD,
        get_outgoing_broadcast_repeat_interval,
        get_wait_incantation_fork_turns,
    )
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
    from broadcast import BROADCAST_INTENTION_INCANTATION, build_plan_from_sound_direction
    from config import (
        ACTION_COOLDOWN_STEPS,
        ALLY_HELP_FOOD_THRESHOLD,
        DEFAULT_OBJECTIVE,
        EXPLORE_TURN_INTERVAL,
        FORCE_OPPORTUNISTIC_AFTER_SAFE_TURNS,
        FORK_FOOD_THRESHOLD,
        FORK_SAFE_TURNS_THRESHOLD,
        INCANTATION_FOOD_THRESHOLD,
        OPPORTUNISTIC_FOOD_THRESHOLD,
        OVERCROWD_EJECT_THRESHOLD,
        PLACE_STONES_ONLY_WHEN_INVENTORY_READY,
        SURVIVAL_FOOD_THRESHOLD,
        get_outgoing_broadcast_repeat_interval,
        get_wait_incantation_fork_turns,
    )
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


@dataclass(frozen=True)
class HeuristicDecision:
    command: str
    rationale: str
    confidence: float
    plan: tuple[str, ...] = ()
    state: str = ""


@dataclass(frozen=True)
class _PredictionContext:
    level: int
    team_name: str
    objective: str
    inventory: dict[str, int]
    reserved_resources: dict[str, int]
    available_slots: int | None
    action_cooldowns: dict[str, int]
    safe_turns: int
    ally_broadcast: dict[str, object] | None
    last_outgoing_broadcast: dict[str, object] | None
    visible_tiles: list[dict[str, object]]
    current_counts: Counter[str]
    needed_stones: set[str]
    inventory_ready_for_elevation: bool
    current_tile_preparation: bool


class _BehaviorState(str, Enum):
    SURVIVE = "survive"
    HELP_INCANTATION = "help_incantation"
    PREPARE_INCANTATION = "prepare_incantation"
    WAIT_INCANTATION = "wait_incantation"
    GATHER = "gather"
    EXPLORE = "explore"


class HeuristicModel:

    def __init__(self) -> None:
        self._state = _BehaviorState.EXPLORE
        self._state_turns = 0
        self._explore_turn_command = "Left"

    def predict(
        self,
        *,
        level: int,
        team_name: str,
        inventory: Mapping[str, int],
        visible_tiles: Sequence[Mapping[str, object]],
        look_is_fresh: bool,
        objective: str = DEFAULT_OBJECTIVE,
        current_tile_reserved_resources: Mapping[str, int] | None = None,
        available_slots: int | None = None,
        action_cooldowns: Mapping[str, int] | None = None,
        safe_turns: int = 0,
        ally_broadcast: Mapping[str, object] | None = None,
        last_outgoing_broadcast: Mapping[str, object] | None = None,
    ) -> HeuristicDecision:
        if not team_name:
            raise ValueError("team_name cannot be empty")

        context = self._build_context(
            level=level,
            team_name=team_name,
            objective=objective,
            inventory=inventory,
            visible_tiles=visible_tiles,
            current_tile_reserved_resources=current_tile_reserved_resources,
            available_slots=available_slots,
            action_cooldowns=action_cooldowns,
            safe_turns=safe_turns,
            ally_broadcast=ally_broadcast,
            last_outgoing_broadcast=last_outgoing_broadcast,
        )

        state = self._resolve_state(context)

        # Refresh vision before committing to the current state.
        if not look_is_fresh or not context.visible_tiles:
            return self._attach_state(
                _build_single_action_decision(
                    command="Look",
                    rationale="vision not fresh or no visible tiles, need to update vision.",
                    confidence=0.99,
                ),
                state,
            )

        decision = self._decide_for_state(state, context)
        return self._attach_state(decision, state)

    def _build_context(
        self,
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
    ) -> _PredictionContext:
        normalized_inventory = normalize_inventory(inventory)
        normalized_reserved_resources = normalize_inventory(current_tile_reserved_resources or {})
        normalized_action_cooldowns = _normalize_action_cooldowns(action_cooldowns or {})
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

    def _resolve_state(self, context: _PredictionContext) -> _BehaviorState:
        # Force a state change only when a stronger objective appears.
        forced_state = self._select_forced_state(context)
        if forced_state is not None:
            return self._activate_state(forced_state)

        # Keep the current state while its objective is still valid.
        if self._state_is_active(self._state, context):
            return self._activate_state(self._state)

        return self._activate_state(self._select_next_state(context))

    def _activate_state(self, next_state: _BehaviorState) -> _BehaviorState:
        if next_state == self._state:
            self._state_turns += 1
            return self._state

        self._state = next_state
        self._state_turns = 1
        return self._state

    def _select_forced_state(self, context: _PredictionContext) -> _BehaviorState | None:
        if self._needs_survival_state(context):
            return _BehaviorState.SURVIVE
        if self._should_wait_for_incantation(context):
            return _BehaviorState.WAIT_INCANTATION
        if self._can_help_ally(context):
            return _BehaviorState.HELP_INCANTATION
        if self._should_prepare_incantation(context):
            return _BehaviorState.PREPARE_INCANTATION
        return None

    def _select_next_state(self, context: _PredictionContext) -> _BehaviorState:
        if self._should_wait_for_incantation(context):
            return _BehaviorState.WAIT_INCANTATION
        if self._can_help_ally(context):
            return _BehaviorState.HELP_INCANTATION
        if self._should_prepare_incantation(context):
            return _BehaviorState.PREPARE_INCANTATION
        if self._should_gather(context):
            return _BehaviorState.GATHER
        return _BehaviorState.EXPLORE

    def _state_is_active(
        self,
        state: _BehaviorState,
        context: _PredictionContext,
    ) -> bool:
        if state == _BehaviorState.SURVIVE:
            return self._needs_survival_state(context)
        if state == _BehaviorState.HELP_INCANTATION:
            return self._can_help_ally(context)
        if state == _BehaviorState.PREPARE_INCANTATION:
            return self._should_prepare_incantation(context)
        if state == _BehaviorState.WAIT_INCANTATION:
            return self._should_wait_for_incantation(context)
        if state == _BehaviorState.GATHER:
            return self._should_gather(context)
        return not self._should_gather(context)

    def _decide_for_state(
        self,
        state: _BehaviorState,
        context: _PredictionContext,
    ) -> HeuristicDecision:
        if state == _BehaviorState.SURVIVE:
            return self._decide_survival_state(context)
        if state == _BehaviorState.HELP_INCANTATION:
            return self._decide_help_incantation_state(context)
        if state == _BehaviorState.PREPARE_INCANTATION:
            return self._decide_prepare_incantation_state(context)
        if state == _BehaviorState.WAIT_INCANTATION:
            return self._decide_wait_incantation_state(context)
        if state == _BehaviorState.GATHER:
            return self._decide_gather_state(context)
        return self._decide_explore_state(context)

    def _attach_state(
        self,
        decision: HeuristicDecision,
        state: _BehaviorState,
    ) -> HeuristicDecision:
        return HeuristicDecision(
            command=decision.command,
            rationale=decision.rationale,
            confidence=decision.confidence,
            plan=decision.plan,
            state=state.value,
        )

    def _needs_survival_state(self, context: _PredictionContext) -> bool:
        return context.inventory["food"] <= SURVIVAL_FOOD_THRESHOLD

    def _should_wait_for_incantation(self, context: _PredictionContext) -> bool:
        tile_ready_for_incantation = _tile_is_ready_for_incantation(
            context.level,
            context.current_counts,
        )
        if self._has_other_ally_incantation_call(context) and not tile_ready_for_incantation:
            return False

        if not _has_enough_food_for_incantation(context.inventory):
            return False
        if not (
            tile_ready_for_incantation
            or context.inventory_ready_for_elevation
        ):
            return False
        return missing_players_for_incantation(context.level, context.current_counts) > 0

    def _should_prepare_incantation(self, context: _PredictionContext) -> bool:
        if not _has_enough_food_for_incantation(context.inventory):
            return False
        if self._should_wait_for_incantation(context):
            return False
        return (
            context.objective == "elevation"
            or context.inventory_ready_for_elevation
            or context.current_tile_preparation
            or _tile_is_ready_for_incantation(context.level, context.current_counts)
        )

    def _should_gather(self, context: _PredictionContext) -> bool:
        if context.needed_stones:
            return True
        return context.inventory["food"] < INCANTATION_FOOD_THRESHOLD

    def _can_help_ally(self, context: _PredictionContext) -> bool:
        ally_broadcast = context.ally_broadcast
        if ally_broadcast is None:
            return False
        if context.inventory["food"] < ALLY_HELP_FOOD_THRESHOLD:
            return False
        if not _has_enough_food_for_incantation(context.inventory):
            return False

        payload = ally_broadcast.get("payload")
        if not isinstance(payload, Mapping):
            return False
        if payload.get("intention") != BROADCAST_INTENTION_INCANTATION:
            return False
        if int(payload.get("level", 0)) != context.level:
            return False
        if _tile_is_ready_for_incantation(context.level, context.current_counts):
            return False
        return True

    def _has_other_ally_incantation_call(self, context: _PredictionContext) -> bool:
        ally_broadcast = context.ally_broadcast
        if ally_broadcast is None:
            return False

        payload = ally_broadcast.get("payload")
        if not isinstance(payload, Mapping):
            return False
        if payload.get("intention") != BROADCAST_INTENTION_INCANTATION:
            return False
        if int(payload.get("level", 0)) != context.level:
            return False
        return int(ally_broadcast.get("direction", 0)) != 0

    def _can_fork(self, context: _PredictionContext) -> bool:
        if context.inventory["food"] < FORK_FOOD_THRESHOLD:
            return False
        return self._cooldown_ready(context, "Fork")

    def _can_eject(self, context: _PredictionContext) -> bool:
        if context.current_counts.get("player", 0) < OVERCROWD_EJECT_THRESHOLD:
            return False
        return self._cooldown_ready(context, "Eject")

    def _decide_survival_state(self, context: _PredictionContext) -> HeuristicDecision:
        decision = self._decide_survival(context)
        if decision is not None:
            return decision
        return _build_single_action_decision(
            command="Forward",
            rationale="food is low and no food is visible, moving to search for food.",
            confidence=0.56,
        )

    def _decide_help_incantation_state(self, context: _PredictionContext) -> HeuristicDecision:
        decision = self._decide_ally_incantation_help(context)
        if decision is not None:
            return decision
        return _build_single_action_decision(
            command="Look",
            rationale="ally incantation target is unclear, refreshing vision before helping again.",
            confidence=0.58,
        )

    def _decide_prepare_incantation_state(self, context: _PredictionContext) -> HeuristicDecision:
        decision = self._decide_elevation(context)
        if decision is not None:
            return decision
        return self._decide_exploration_with_rationale(
            "still preparing the next incantation, exploring to find the missing stones.",
        )

    def _decide_wait_incantation_state(self, context: _PredictionContext) -> HeuristicDecision:
        return self._decide_waiting_for_players(context)

    def _decide_gather_state(self, context: _PredictionContext) -> HeuristicDecision:
        extra_action = self._decide_safe_extra_action(context)
        if extra_action is not None:
            return extra_action

        decision = self._decide_general_collection(context)
        if decision is not None:
            return decision

        return self._decide_exploration_with_rationale(
            "still gathering resources for the next level, exploring to find useful tiles.",
        )

    def _decide_explore_state(self, context: _PredictionContext) -> HeuristicDecision:
        extra_action = self._decide_safe_extra_action(context)
        if extra_action is not None:
            return extra_action
        return self._decide_exploration()

    def _decide_safe_extra_action(
        self,
        context: _PredictionContext,
    ) -> HeuristicDecision | None:
        if (
            context.safe_turns >= FORK_SAFE_TURNS_THRESHOLD
            and context.inventory["food"] >= OPPORTUNISTIC_FOOD_THRESHOLD
            and self._can_fork(context)
        ):
            return _build_single_action_decision(
                command="Fork",
                rationale="the player has been safe for a while and has enough food to grow the team.",
                confidence=0.58,
            )

        if context.safe_turns < FORCE_OPPORTUNISTIC_AFTER_SAFE_TURNS:
            return None
        if context.inventory["food"] < OPPORTUNISTIC_FOOD_THRESHOLD:
            return None

        if self._can_eject(context):
            return _build_single_action_decision(
                command="Eject",
                rationale="the tile stays crowded even in a safe state, ejecting can break the local loop.",
                confidence=0.46,
            )

        return None

    def _decide_exploration_with_rationale(self, rationale: str) -> HeuristicDecision:
        direction_decision = self._decide_exploration()
        return HeuristicDecision(
            command=direction_decision.command,
            rationale=rationale,
            confidence=direction_decision.confidence,
            plan=direction_decision.plan,
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
        if not _has_enough_food_for_incantation(context.inventory):
            return None

        tile_ready_for_incantation = _tile_is_ready_for_incantation(
            context.level,
            context.current_counts,
        )
        can_place_stones = context.current_tile_preparation or context.inventory_ready_for_elevation
        if not PLACE_STONES_ONLY_WHEN_INVENTORY_READY:
            can_place_stones = True

        if can_place_stones:
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
            or tile_ready_for_incantation
        )
        if not should_prepare_elevation:
            return None

        if tile_ready_for_incantation or context.inventory_ready_for_elevation:
            missing_players = missing_players_for_incantation(
                context.level,
                context.current_counts,
            )
            if missing_players > 0:
                return self._decide_waiting_for_players(context)

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
            reserved_resources=context.reserved_resources,
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

    def _decide_ally_incantation_help(self, context: _PredictionContext) -> HeuristicDecision | None:
        if not self._can_help_ally(context):
            return None
        ally_broadcast = context.ally_broadcast
        if ally_broadcast is None:
            return None

        direction = int(ally_broadcast.get("direction", -1))
        if direction == 0:
            return _build_single_action_decision(
                command="Look",
                rationale="already on the ally incantation tile, staying available to help.",
                confidence=0.73,
            )

        plan = build_plan_from_sound_direction(direction)
        if not plan:
            return None

        command = plan[0]
        return HeuristicDecision(
            command=command,
            rationale=(
                "an allied incantation broadcast was received and the player is eligible to help. "
                f"Following the sound direction {direction}."
            ),
            confidence=0.76,
            plan=plan,
        )

    def _decide_waiting_for_players(self, context: _PredictionContext) -> HeuristicDecision:
        if (
            self._cooldown_ready(context, "Broadcast")
            and self._should_send_incantation_broadcast(context)
        ):
            return _build_single_action_decision(
                command="Broadcast incantation",
                rationale=(
                    "inventory ready for the next incantation but not enough players "
                    "are currently on the tile."
                ),
                confidence=0.72,
            )

        if (
            context.safe_turns >= get_wait_incantation_fork_turns(context.level)
            and self._can_fork(context)
        ):
            return _build_single_action_decision(
                command="Fork",
                rationale="waiting for more players on the incantation tile, creating an egg can help future regrouping.",
                confidence=0.60,
            )

        return _build_single_action_decision(
            command="Look",
            rationale="waiting on the tile while staying ready for the incantation.",
            confidence=0.45,
        )

    def _decide_exploration(self) -> HeuristicDecision:
        if self._state_turns % EXPLORE_TURN_INTERVAL == 0:
            command = self._explore_turn_command
            self._explore_turn_command = "Right" if command == "Left" else "Left"
            return _build_single_action_decision(
                command=command,
                rationale="No useful resources in sight, making a periodic turn to avoid a straight loop.",
                confidence=0.34,
            )

        return _build_single_action_decision(
            command="Forward",
            rationale="No useful resources in sight, moving forward to discover new tiles.",
            confidence=0.62,
        )

    def _cooldown_ready(self, context: _PredictionContext, action_name: str) -> bool:
        return context.action_cooldowns.get(action_name, 0) <= 0

    def _should_send_incantation_broadcast(self, context: _PredictionContext) -> bool:
        if self._has_other_ally_incantation_call(context):
            return False

        last_outgoing_broadcast = context.last_outgoing_broadcast
        repeat_interval = get_outgoing_broadcast_repeat_interval(context.level)
        if last_outgoing_broadcast is None:
            return True
        if last_outgoing_broadcast.get("intention") != BROADCAST_INTENTION_INCANTATION:
            return True
        if int(last_outgoing_broadcast.get("level", 0)) != context.level:
            return True
        return int(last_outgoing_broadcast.get("age", repeat_interval)) >= repeat_interval

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


def _normalize_action_cooldowns(action_cooldowns: Mapping[str, int]) -> dict[str, int]:
    normalized = {action_name: 0 for action_name in ACTION_COOLDOWN_STEPS}
    for action_name, value in action_cooldowns.items():
        normalized[str(action_name)] = int(value)
    return normalized


def _tile_is_ready_for_incantation(
    level: int,
    current_counts: Counter[str],
) -> bool:
    requirement = LEVEL_REQUIREMENTS.get(level)
    if requirement is None:
        return False
    return ground_matches_requirement(current_counts, requirement["stones"])


def _has_enough_food_for_incantation(inventory: Mapping[str, int]) -> bool:
    return int(inventory.get("food", 0)) >= INCANTATION_FOOD_THRESHOLD


def _decide_incantation_step(
    level: int,
    inventory: Mapping[str, int],
    current_counts: Counter[str],
) -> HeuristicDecision | None:
    requirement = LEVEL_REQUIREMENTS.get(level)
    if requirement is None:
        return None
    if not _has_enough_food_for_incantation(inventory):
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
