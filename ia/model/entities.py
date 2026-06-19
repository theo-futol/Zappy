"""Shared state and decision objects for the heuristic model."""

from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
from enum import Enum


@dataclass(frozen=True)
class HeuristicDecision:
    command: str
    rationale: str
    confidence: float
    plan: tuple[str, ...] = ()
    state: str = ""


@dataclass(frozen=True)
class PredictionContext:
    level: int
    team_name: str
    objective: str
    inventory: dict[str, int]
    reserved_resources: dict[str, int]
    available_slots: int | None
    action_cooldowns: dict[str, int]
    safe_turns: int
    ally_broadcast: dict[str, object] | None
    incantation_support: dict[str, bool | int]
    last_outgoing_broadcast: dict[str, object] | None
    visible_tiles: list[dict[str, object]]
    current_counts: Counter[str]
    needed_stones: set[str]
    inventory_ready_for_elevation: bool
    current_tile_preparation: bool


class BehaviorState(str, Enum):
    SURVIVE = "survive"
    HELP_INCANTATION = "help_incantation"
    PREPARE_INCANTATION = "prepare_incantation"
    WAIT_INCANTATION = "wait_incantation"
    GATHER = "gather"
    EXPLORE = "explore"
