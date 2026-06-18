"""Deterministic finite-state model for a Zappy AI client."""

from __future__ import annotations

from typing import Mapping, Sequence

try:
    from .config import DEFAULT_OBJECTIVE
    from .model import (
        BehaviorStateMachine,
        HeuristicDecision,
        build_prediction_context,
        build_single_action_decision,
    )
except ImportError:
    from config import DEFAULT_OBJECTIVE
    from model import (
        BehaviorStateMachine,
        HeuristicDecision,
        build_prediction_context,
        build_single_action_decision,
    )


class HeuristicModel:

    def __init__(self) -> None:
        self._state_machine = BehaviorStateMachine()

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

        context = build_prediction_context(
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

        state = self._state_machine.resolve_state(context)

        # Refresh vision before committing to the current state.
        if not look_is_fresh or not context.visible_tiles:
            return self._state_machine.attach_state(
                build_single_action_decision(
                    command="Look",
                    rationale="vision not fresh or no visible tiles, need to update vision.",
                    confidence=0.99,
                ),
                state,
            )

        decision = self._state_machine.decide_for_state(state, context)
        return self._state_machine.attach_state(decision, state)
