"""Flat finite-state strategy for a single Zappy AI player.

Each turn rebuilds a fresh Observation (Look + Inventory), picks exactly one
state, and runs that state's body. There is no shared mutable booleans
threaded across functions: every state body only reads the Observation and
a handful of plain attributes on the strategy itself (rally tracking,
cooldowns).

Coordination model: a player who has decent food and at least one stone
useful for its next elevation broadcasts a "ready" ping that lists which of
those stones it carries. Every player tracks the ready pings it has heard
for its own level and builds a predicted pooled inventory from them. Only
once enough ready players are known AND their combined stones (plus our own)
would satisfy the elevation does anyone actually move to converge - this
avoids dragging people across the whole map towards a group that turns out
to be incomplete once everyone arrives.
"""

from __future__ import annotations

import secrets
from collections import Counter
from enum import Enum

try:
    from .broadcast import build_plan_from_sound_direction, build_rally_message, parse_rally_message
    from .config import (
        BROADCAST_COOLDOWN_TURNS,
        EXPLORE_TURN_INTERVAL,
        FORK_COOLDOWN_TURNS,
        FORK_FOOD_THRESHOLD,
        GATHER_FOOD_THRESHOLD,
        RALLY_CALL_EXPIRY_TURNS,
        RALLY_GIVE_UP_TURNS,
        SURVIVAL_FOOD_THRESHOLD,
    )
    from .observation import Observation
    from .utils.model_utils import (
        LEVEL_REQUIREMENTS,
        build_plan_to_tile,
        ground_matches_requirement,
        pick_current_tile_resource,
        select_visible_target,
    )
except ImportError:
    from broadcast import build_plan_from_sound_direction, build_rally_message, parse_rally_message
    from config import (
        BROADCAST_COOLDOWN_TURNS,
        EXPLORE_TURN_INTERVAL,
        FORK_COOLDOWN_TURNS,
        FORK_FOOD_THRESHOLD,
        GATHER_FOOD_THRESHOLD,
        RALLY_CALL_EXPIRY_TURNS,
        RALLY_GIVE_UP_TURNS,
        SURVIVAL_FOOD_THRESHOLD,
    )
    from observation import Observation
    from utils.model_utils import (
        LEVEL_REQUIREMENTS,
        build_plan_to_tile,
        ground_matches_requirement,
        pick_current_tile_resource,
        select_visible_target,
    )


class State(Enum):
    SURVIVING = "surviving"
    GATHERING = "gathering"
    WAITING = "waiting"
    INCANTING = "incanting"
    JOINING = "joining"
    EXPLORING = "exploring"


_MOVE_COMMANDS = {
    "Forward": lambda client: client.forward(),
    "Left": lambda client: client.left(),
    "Right": lambda client: client.right(),
}


class Strategy:

    def __init__(self, client, team: str, level: int = 1) -> None:
        self.client = client
        self.team = team
        self.level = level
        self.state = State.EXPLORING

        self.token = secrets.token_hex(4)
        # token -> {"direction": int, "age": int, "resources": dict[str, int]}
        self.ready_seen: dict[str, dict[str, object]] = {}
        self.rallying = False
        self.rally_turns = 0
        # With many ready players broadcasting at once, always following
        # whichever ping is freshest makes the target jump every turn and
        # never converges on anyone. Lock onto one token while rallying.
        self.target_token: str | None = None

        self.broadcast_cooldown = 0
        self.fork_cooldown = 0
        self._explore_tick = 0
        self._explore_turn_command = "Left"

    def step(self) -> bool:
        if self.client.pending_level is not None:
            # Pushed by the server because we were a participant in someone
            # else's successful incantation, not the one who triggered it.
            self.level = self.client.pending_level
            self.client.pending_level = None
            self._clear_rally()

        obs = Observation(self.client, self.level)
        if obs.dead:
            return False

        self._age_ready_seen()
        self._process_messages(obs)
        self.broadcast_cooldown = max(0, self.broadcast_cooldown - 1)
        self.fork_cooldown = max(0, self.fork_cooldown - 1)

        self.state = self._select_state(obs)
        self._run_state(obs)
        return True

    # -- state selection ----------------------------------------------------

    def _select_state(self, obs: Observation) -> State:
        if obs.food <= SURVIVAL_FOOD_THRESHOLD:
            # Food always wins: a stale rally is worthless to a dead player.
            self._clear_rally()
            return State.SURVIVING

        required = self._required_players()

        local_group_threshold = self._local_group_threshold(required)

        if not self.rallying and obs.players_here >= local_group_threshold:
            # Already standing next to someone (often survivors of the
            # incantation that just leveled us up): try to consolidate right
            # here first, no broadcast or travel needed for that.
            self.rallying = True
            self.rally_turns = 0
            self.target_token = self.token
        elif not self.rallying and self._is_ready(obs) and self._enough_to_converge(obs, required):
            self.rallying = True
            self.rally_turns = 0
            self.target_token = self._pick_target_token()

        if self.rallying:
            direction = self._target_direction()
            if direction is None:
                # Lost track of our target (its ping expired): try another
                # known one rather than giving up immediately.
                self.target_token = self._pick_target_token()
                direction = self._target_direction()
            # "Arrived" means physically grouped with at least someone, not
            # the full quorum: stop and let the cluster grow from here
            # instead of requiring every member to land on the same tile in
            # the same instant, which never happens with direction-only nav.
            fresh_target_bearing = self._target_bearing_is_fresh()
            partial_group = obs.players_here >= local_group_threshold and not fresh_target_bearing
            arrived = direction is None or direction == 0 or partial_group
            if arrived and self._ready_for_incantation(obs) and obs.players_here >= required:
                return State.INCANTING

            self.rally_turns += 1
            if self.rally_turns > self._rally_give_up_limit(required):
                # No distance information exists in a broadcast: the pooled
                # group turned out to be unreachable, or short a resource
                # nobody actually had. Stop chasing it and start fresh.
                self._clear_rally()
            elif arrived:
                return State.WAITING
            else:
                return State.JOINING

        if self._ready_for_incantation(obs) and obs.players_here >= required:
            return State.INCANTING
        return State.GATHERING

    def _clear_rally(self) -> None:
        self.rallying = False
        self.rally_turns = 0
        self.target_token = None
        self.ready_seen.clear()

    def _run_state(self, obs: Observation) -> None:
        if self.state == State.SURVIVING:
            self._do_surviving(obs)
        elif self.state == State.JOINING:
            self._do_joining(obs)
        elif self.state == State.WAITING:
            self._do_waiting(obs)
        elif self.state == State.INCANTING:
            self._do_incanting(obs)
        elif self.state == State.GATHERING:
            self._do_gathering(obs)
        else:
            self._do_exploring()

    def _ready_for_incantation(self, obs: Observation) -> bool:
        requirement = LEVEL_REQUIREMENTS.get(self.level)
        if requirement is None:
            return False
        if obs.ready_for_elevation:
            return True
        # Stones already pooled/left on the ground count too.
        return ground_matches_requirement(obs.current_counts, requirement["stones"])

    def _required_players(self) -> int:
        requirement = LEVEL_REQUIREMENTS.get(self.level)
        return int(requirement["players"]) if requirement else 1

    def _local_group_threshold(self, required_players: int) -> int:
        if required_players >= 6:
            # At high levels, stopping on every pair fragments the swarm into
            # tiny camps. A 3+ cluster is worth preserving and growing.
            return max(2, required_players // 2)
        return 2

    def _rally_give_up_limit(self, required_players: int) -> int:
        if required_players >= 6:
            return RALLY_GIVE_UP_TURNS * 2
        return RALLY_GIVE_UP_TURNS

    def _is_ready(self, obs: Observation) -> bool:
        requirement = LEVEL_REQUIREMENTS.get(self.level)
        if requirement is None or obs.food < GATHER_FOOD_THRESHOLD:
            return False
        return any(obs.inv.get(stone, 0) > 0 for stone in requirement["stones"])

    def _own_contribution(self, obs: Observation) -> dict[str, int]:
        requirement = LEVEL_REQUIREMENTS.get(self.level)
        if requirement is None:
            return {}
        return {
            stone: min(obs.inv.get(stone, 0), amount)
            for stone, amount in requirement["stones"].items()
            if obs.inv.get(stone, 0) > 0
        }

    def _enough_to_converge(self, obs: Observation, required_players: int) -> bool:
        if len(self.ready_seen) + 1 < required_players:
            return False
        requirement = LEVEL_REQUIREMENTS.get(self.level)
        if requirement is None:
            return False
        pooled = Counter(self._own_contribution(obs))
        for info in self.ready_seen.values():
            pooled.update(info["resources"])
        return all(pooled.get(stone, 0) >= amount for stone, amount in requirement["stones"].items())

    def _pick_target_token(self) -> str:
        # A deterministic rule (not "whoever I heard most recently") so that
        # everyone in the same ready cohort independently agrees on the same
        # single meeting point instead of each locking onto a different peer.
        return min([*self.ready_seen, self.token])

    def _target_direction(self) -> int | None:
        if self.target_token is None:
            return None
        if self.target_token == self.token:
            return 0
        info = self.ready_seen.get(self.target_token)
        return int(info["direction"]) if info else None

    # -- rally tracking -------------------------------------------------------

    def _process_messages(self, obs: Observation) -> None:
        for direction, text in obs.messages:
            parsed = parse_rally_message(text)
            if parsed is None:
                continue
            msg_level, msg_team, msg_token, msg_resources = parsed
            if msg_team != self.team or msg_level != self.level or msg_token == self.token:
                continue
            self.ready_seen[msg_token] = {
                "direction": direction,
                "age": 0,
                "resources": msg_resources,
            }

    def _age_ready_seen(self) -> None:
        expired = [
            token
            for token, info in self.ready_seen.items()
            if int(info["age"]) + 1 > RALLY_CALL_EXPIRY_TURNS
        ]
        for token in expired:
            del self.ready_seen[token]
        for info in self.ready_seen.values():
            info["age"] = int(info["age"]) + 1

    # -- state bodies ---------------------------------------------------------

    def _do_surviving(self, obs: Observation) -> None:
        if obs.on_my_tile("food"):
            self.client.take("food")
            return
        target = select_visible_target(obs.tiles, {"food"})
        if target is not None:
            self._move_to(target)
            return
        # No food in sight: scan while moving instead of committing to one
        # straight line, vision is too narrow to risk walking away from food.
        self._do_exploring()

    def _do_gathering(self, obs: Observation) -> None:
        # Forking and advertising readiness are cheap side actions, not
        # exclusive with the main gathering move below in the same turn.
        self._maybe_fork(obs)
        if self._is_ready(obs):
            self._send_ready_ping(obs)

        # Below the gather threshold, food takes priority over stones even
        # when a stone is closer: running out mid-stone-hunt is worse.
        if obs.food < GATHER_FOOD_THRESHOLD:
            if obs.on_my_tile("food"):
                self.client.take("food")
                return
            target = select_visible_target(obs.tiles, {"food"})
            if target is not None:
                self._move_to(target)
                return

        resource = pick_current_tile_resource(obs.current_counts, obs.needed_stones | {"food"})
        if resource is not None:
            self.client.take(resource)
            return
        target = select_visible_target(obs.tiles, obs.needed_stones | {"food"})
        if target is not None:
            self._move_to(target)
            return
        self._do_exploring()

    def _send_ready_ping(self, obs: Observation) -> None:
        if self.broadcast_cooldown > 0:
            return
        message = build_rally_message(
            level=self.level,
            team=self.team,
            token=self.token,
            resources=self._own_contribution(obs),
        )
        self.client.broadcast(message)
        self.broadcast_cooldown = BROADCAST_COOLDOWN_TURNS

    def _do_waiting(self, obs: Observation) -> None:
        self._take_food_if_here(obs)
        # Keep advertising our position while waiting (even once everything
        # has already been deposited): stragglers still need a fresh bearing
        # to navigate by, and our entry would otherwise expire for them.
        self._send_ready_ping(obs)
        requirement = LEVEL_REQUIREMENTS.get(self.level)
        if requirement is None:
            return
        required_players = int(requirement["players"])
        if required_players >= 6 and obs.players_here < required_players:
            # Keep stones in inventories while a high-level rally is still
            # forming. Dropping them into 2-4 player camps makes broadcasts
            # turn into "none" and strands resources across the map.
            return
        for stone, amount in requirement["stones"].items():
            missing_on_ground = amount - obs.current_counts.get(stone, 0)
            to_place = max(0, min(missing_on_ground, obs.inv.get(stone, 0)))
            for _ in range(to_place):
                self.client.set_obj(stone)

    def _do_incanting(self, obs: Observation) -> None:
        requirement = LEVEL_REQUIREMENTS[self.level]
        for stone, amount in requirement["stones"].items():
            missing = amount - obs.current_counts.get(stone, 0)
            for _ in range(max(0, missing)):
                self.client.set_obj(stone)

        new_level = self.client.incantation()
        if new_level is not None:
            self.level = new_level
            self._clear_rally()
        # On failure the stones stay on the ground; next turn's fresh
        # Observation re-evaluates readiness from scratch.

    def _do_joining(self, obs: Observation) -> None:
        # Grab food and any useful stone in passing, but never detour off
        # the path to the rally: only what's already on the current tile.
        self._take_food_if_here(obs)
        resource = pick_current_tile_resource(obs.current_counts, obs.needed_stones)
        if resource is not None:
            self.client.take(resource)
        direction = self._target_direction()
        if direction is None:
            return

        if self._target_bearing_is_fresh():
            # A direction is only meaningful relative to our facing at the
            # moment it was heard. Re-applying the same turn+move every turn
            # without a new ping would spin in place instead of approaching.
            for command in build_plan_from_sound_direction(direction):
                _MOVE_COMMANDS[command](self.client)
        else:
            self.client.forward()

    def _target_bearing_is_fresh(self) -> bool:
        if self.target_token == self.token:
            return False
        info = self.ready_seen.get(self.target_token)
        return info is not None and int(info["age"]) == 0

    def _take_food_if_here(self, obs: Observation) -> None:
        if obs.on_my_tile("food"):
            self.client.take("food")

    def _do_exploring(self) -> None:
        self._explore_tick += 1
        if self._explore_tick % EXPLORE_TURN_INTERVAL == 0:
            command = self._explore_turn_command
            self._explore_turn_command = "Right" if command == "Left" else "Left"
            _MOVE_COMMANDS[command](self.client)
            return
        self.client.forward()

    def _maybe_fork(self, obs: Observation) -> bool:
        if self.fork_cooldown > 0 or obs.food < FORK_FOOD_THRESHOLD:
            return False
        # An unhatched egg is already waiting: laying another one would just
        # pile up eggs nobody connects to (the launcher caps client count).
        if self.client.connect_nbr() > 0:
            self.fork_cooldown = FORK_COOLDOWN_TURNS
            return False
        self.client.fork()
        self.fork_cooldown = FORK_COOLDOWN_TURNS
        return True

    def _move_to(self, target: dict[str, object]) -> None:
        for command in build_plan_to_tile(target):
            _MOVE_COMMANDS[command](self.client)
