"""Finite-state decision logic for the Zappy heuristic model."""

from __future__ import annotations

from typing import Mapping

try:
    from ..broadcast import BROADCAST_INTENTION_INCANTATION, build_plan_from_sound_direction
    from ..config import (
        ALLY_HELP_FOOD_THRESHOLD,
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
    from ..utils.model_utils import (
        pick_current_tile_resource,
        missing_players_for_incantation,
        select_visible_target,
    )
    from .decision_support import (
        build_single_action_decision,
        decide_incantation_step,
        has_enough_food_for_incantation,
        move_towards,
        tile_is_ready_for_incantation,
    )
    from .entities import BehaviorState, HeuristicDecision, PredictionContext
except ImportError:
    from broadcast import BROADCAST_INTENTION_INCANTATION, build_plan_from_sound_direction
    from config import (
        ALLY_HELP_FOOD_THRESHOLD,
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
    from model.decision_support import (
        build_single_action_decision,
        decide_incantation_step,
        has_enough_food_for_incantation,
        move_towards,
        tile_is_ready_for_incantation,
    )
    from model.entities import BehaviorState, HeuristicDecision, PredictionContext
    from utils.model_utils import (
        pick_current_tile_resource,
        missing_players_for_incantation,
        select_visible_target,
    )


class BehaviorStateMachine:

    def __init__(self) -> None:
        self._state = BehaviorState.EXPLORE
        self._state_turns = 0
        self._explore_turn_command = "Left"

    def resolve_state(self, context: PredictionContext) -> BehaviorState:
        forced_state = self._select_forced_state(context)
        if forced_state is not None:
            return self._activate_state(forced_state)

        if self._state_is_active(self._state, context):
            return self._activate_state(self._state)

        return self._activate_state(self._select_next_state(context))

    def decide_for_state(
        self,
        state: BehaviorState,
        context: PredictionContext,
    ) -> HeuristicDecision:
        if state == BehaviorState.SURVIVE:
            return self._decide_survival_state(context)
        if state == BehaviorState.HELP_INCANTATION:
            return self._decide_help_incantation_state(context)
        if state == BehaviorState.PREPARE_INCANTATION:
            return self._decide_prepare_incantation_state(context)
        if state == BehaviorState.WAIT_INCANTATION:
            return self._decide_wait_incantation_state(context)
        if state == BehaviorState.GATHER:
            return self._decide_gather_state(context)
        return self._decide_explore_state(context)

    def attach_state(
        self,
        decision: HeuristicDecision,
        state: BehaviorState,
    ) -> HeuristicDecision:
        return HeuristicDecision(
            command=decision.command,
            rationale=decision.rationale,
            confidence=decision.confidence,
            plan=decision.plan,
            state=state.value,
        )

    def _activate_state(self, next_state: BehaviorState) -> BehaviorState:
        if next_state == self._state:
            self._state_turns += 1
            return self._state

        self._state = next_state
        self._state_turns = 1
        return self._state

    def _select_forced_state(self, context: PredictionContext) -> BehaviorState | None:
        if self._needs_survival_state(context):
            return BehaviorState.SURVIVE
        if self._should_wait_for_incantation(context):
            return BehaviorState.WAIT_INCANTATION
        if self._can_help_ally(context):
            return BehaviorState.HELP_INCANTATION
        if self._should_prepare_incantation(context):
            return BehaviorState.PREPARE_INCANTATION
        return None

    def _select_next_state(self, context: PredictionContext) -> BehaviorState:
        if self._should_wait_for_incantation(context):
            return BehaviorState.WAIT_INCANTATION
        if self._can_help_ally(context):
            return BehaviorState.HELP_INCANTATION
        if self._should_prepare_incantation(context):
            return BehaviorState.PREPARE_INCANTATION
        if self._should_gather(context):
            return BehaviorState.GATHER
        return BehaviorState.EXPLORE

    def _state_is_active(
        self,
        state: BehaviorState,
        context: PredictionContext,
    ) -> bool:
        if state == BehaviorState.SURVIVE:
            return self._needs_survival_state(context)
        if state == BehaviorState.HELP_INCANTATION:
            return self._can_help_ally(context)
        if state == BehaviorState.PREPARE_INCANTATION:
            return self._should_prepare_incantation(context)
        if state == BehaviorState.WAIT_INCANTATION:
            return self._should_wait_for_incantation(context)
        if state == BehaviorState.GATHER:
            return self._should_gather(context)
        return not self._should_gather(context)

    def _needs_survival_state(self, context: PredictionContext) -> bool:
        return context.inventory["food"] <= SURVIVAL_FOOD_THRESHOLD

    def _should_wait_for_incantation(self, context: PredictionContext) -> bool:
        tile_ready_for_incantation = tile_is_ready_for_incantation(
            context.level,
            context.current_counts,
        )
        if self._has_other_ally_incantation_call(context) and not tile_ready_for_incantation:
            return False

        if not has_enough_food_for_incantation(context.inventory):
            return False
        if not (tile_ready_for_incantation or context.inventory_ready_for_elevation):
            return False
        return missing_players_for_incantation(context.level, context.current_counts) > 0

    def _should_prepare_incantation(self, context: PredictionContext) -> bool:
        if not has_enough_food_for_incantation(context.inventory):
            return False
        if self._should_wait_for_incantation(context):
            return False
        return (
            context.objective == "elevation"
            or context.inventory_ready_for_elevation
            or context.current_tile_preparation
            or tile_is_ready_for_incantation(context.level, context.current_counts)
        )

    def _should_gather(self, context: PredictionContext) -> bool:
        if context.needed_stones:
            return True
        return context.inventory["food"] < INCANTATION_FOOD_THRESHOLD

    def _can_help_ally(self, context: PredictionContext) -> bool:
        ally_broadcast = context.ally_broadcast
        if ally_broadcast is None:
            return False
        if context.inventory["food"] < ALLY_HELP_FOOD_THRESHOLD:
            return False
        if not has_enough_food_for_incantation(context.inventory):
            return False

        payload = ally_broadcast.get("payload")
        if not isinstance(payload, Mapping):
            return False
        if payload.get("intention") != BROADCAST_INTENTION_INCANTATION:
            return False
        if int(payload.get("level", 0)) != context.level:
            return False
        if tile_is_ready_for_incantation(context.level, context.current_counts):
            return False
        return True

    def _has_other_ally_incantation_call(self, context: PredictionContext) -> bool:
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

    def _can_fork(self, context: PredictionContext) -> bool:
        if context.inventory["food"] < FORK_FOOD_THRESHOLD:
            return False
        return self._cooldown_ready(context, "Fork")

    def _can_eject(self, context: PredictionContext) -> bool:
        if context.current_counts.get("player", 0) < OVERCROWD_EJECT_THRESHOLD:
            return False
        return self._cooldown_ready(context, "Eject")

    def _decide_survival_state(self, context: PredictionContext) -> HeuristicDecision:
        decision = self._decide_survival(context)
        if decision is not None:
            return decision
        return build_single_action_decision(
            command="Forward",
            rationale="food is low and no food is visible, moving to search for food.",
            confidence=0.56,
        )

    def _decide_help_incantation_state(self, context: PredictionContext) -> HeuristicDecision:
        decision = self._decide_ally_incantation_help(context)
        if decision is not None:
            return decision
        return build_single_action_decision(
            command="Look",
            rationale="ally incantation target is unclear, refreshing vision before helping again.",
            confidence=0.58,
        )

    def _decide_prepare_incantation_state(self, context: PredictionContext) -> HeuristicDecision:
        decision = self._decide_elevation(context)
        if decision is not None:
            return decision
        return self._decide_exploration_with_rationale(
            "still preparing the next incantation, exploring to find the missing stones.",
        )

    def _decide_wait_incantation_state(self, context: PredictionContext) -> HeuristicDecision:
        return self._decide_waiting_for_players(context)

    def _decide_gather_state(self, context: PredictionContext) -> HeuristicDecision:
        extra_action = self._decide_safe_extra_action(context)
        if extra_action is not None:
            return extra_action

        decision = self._decide_general_collection(context)
        if decision is not None:
            return decision

        return self._decide_exploration_with_rationale(
            "still gathering resources for the next level, exploring to find useful tiles.",
        )

    def _decide_explore_state(self, context: PredictionContext) -> HeuristicDecision:
        extra_action = self._decide_safe_extra_action(context)
        if extra_action is not None:
            return extra_action
        return self._decide_exploration()

    def _decide_safe_extra_action(self, context: PredictionContext) -> HeuristicDecision | None:
        if (
            context.safe_turns >= FORK_SAFE_TURNS_THRESHOLD
            and context.inventory["food"] >= OPPORTUNISTIC_FOOD_THRESHOLD
            and self._can_fork(context)
        ):
            return build_single_action_decision(
                command="Fork",
                rationale="the player has been safe for a while and has enough food to grow the team.",
                confidence=0.58,
            )

        if context.safe_turns < FORCE_OPPORTUNISTIC_AFTER_SAFE_TURNS:
            return None
        if context.inventory["food"] < OPPORTUNISTIC_FOOD_THRESHOLD:
            return None

        if self._can_eject(context):
            return build_single_action_decision(
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

    def _decide_survival(self, context: PredictionContext) -> HeuristicDecision | None:
        if context.inventory["food"] > SURVIVAL_FOOD_THRESHOLD:
            return None

        if context.current_counts["food"] > 0:
            return build_single_action_decision(
                command="Take food",
                rationale="food is on the current tile, need to take it to survive.",
                confidence=0.98,
            )

        target = select_visible_target(context.visible_tiles, {"food"})
        if target is None:
            return None
        return move_towards(target, "food is prioritized for survival.")

    def _decide_elevation(self, context: PredictionContext) -> HeuristicDecision | None:
        if not has_enough_food_for_incantation(context.inventory):
            return None

        tile_ready_for_incantation = tile_is_ready_for_incantation(
            context.level,
            context.current_counts,
        )
        can_place_stones = context.current_tile_preparation or context.inventory_ready_for_elevation
        if not PLACE_STONES_ONLY_WHEN_INVENTORY_READY:
            can_place_stones = True

        if can_place_stones:
            ritual_command = decide_incantation_step(
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
            return build_single_action_decision(
                command=f"Take {current_stone}",
                rationale=f"current content {current_stone}, can be useful for the incantation.",
                confidence=0.9,
            )

        target = select_visible_target(context.visible_tiles, context.needed_stones)
        if target is None:
            return None
        return move_towards(target, "stone needed for incantation is visible, moving towards it.")

    def _decide_general_collection(self, context: PredictionContext) -> HeuristicDecision | None:
        if context.current_counts["food"] > 0:
            return build_single_action_decision(
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
            return build_single_action_decision(
                command=f"Take {current_stone}",
                rationale=f"current content {current_stone}, still needed for the next incantation.",
                confidence=0.7,
            )

        target = select_visible_target(context.visible_tiles, context.needed_stones | {"food"})
        if target is None:
            return None
        return move_towards(target, "A useful resource is visible, moving towards it.")

    def _decide_ally_incantation_help(self, context: PredictionContext) -> HeuristicDecision | None:
        if not self._can_help_ally(context):
            return None
        ally_broadcast = context.ally_broadcast
        if ally_broadcast is None:
            return None

        direction = int(ally_broadcast.get("direction", -1))
        if direction == 0:
            return build_single_action_decision(
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

    def _decide_waiting_for_players(self, context: PredictionContext) -> HeuristicDecision:
        if (
            self._cooldown_ready(context, "Broadcast")
            and self._should_send_incantation_broadcast(context)
        ):
            return build_single_action_decision(
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
            return build_single_action_decision(
                command="Fork",
                rationale="waiting for more players on the incantation tile, creating an egg can help future regrouping.",
                confidence=0.60,
            )

        return build_single_action_decision(
            command="Look",
            rationale="waiting on the tile while staying ready for the incantation.",
            confidence=0.45,
        )

    def _decide_exploration(self) -> HeuristicDecision:
        if self._state_turns % EXPLORE_TURN_INTERVAL == 0:
            command = self._explore_turn_command
            self._explore_turn_command = "Right" if command == "Left" else "Left"
            return build_single_action_decision(
                command=command,
                rationale="No useful resources in sight, making a periodic turn to avoid a straight loop.",
                confidence=0.34,
            )

        return build_single_action_decision(
            command="Forward",
            rationale="No useful resources in sight, moving forward to discover new tiles.",
            confidence=0.62,
        )

    def _cooldown_ready(self, context: PredictionContext, action_name: str) -> bool:
        return context.action_cooldowns.get(action_name, 0) <= 0

    def _should_send_incantation_broadcast(self, context: PredictionContext) -> bool:
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
