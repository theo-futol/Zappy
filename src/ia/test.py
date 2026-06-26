#!/usr/bin/env python3
"""Manual sanity checks for the Strategy FSM, no live server required."""

from __future__ import annotations

try:
    from strategy import Strategy
except ImportError:
    from .strategy import Strategy


class FakeClient:
    """Stands in for ZappyAIClient: records calls, returns canned answers."""

    def __init__(self, *, look_tiles: list[list[str]], inventory: dict[str, int]) -> None:
        self.dead = False
        self._look_tiles = look_tiles
        self._inventory = inventory
        self.calls: list[str] = []
        self.incantation_result: int | None = None
        self.available_slots = 0
        self.pending_level: int | None = None
        self._messages: list[tuple[int, str]] = []

    def forward(self) -> str:
        self.calls.append("Forward")
        return "ok"

    def left(self) -> str:
        self.calls.append("Left")
        return "ok"

    def right(self) -> str:
        self.calls.append("Right")
        return "ok"

    def take(self, item: str) -> str:
        self.calls.append(f"Take {item}")
        return "ok"

    def set_obj(self, item: str) -> str:
        self.calls.append(f"Set {item}")
        return "ok"

    def fork(self) -> str:
        self.calls.append("Fork")
        return "ok"

    def connect_nbr(self) -> int:
        self.calls.append("Connect_nbr")
        return self.available_slots

    def eject(self) -> str:
        self.calls.append("Eject")
        return "ok"

    def broadcast(self, text: str) -> str:
        self.calls.append(f"Broadcast {text}")
        return "ok"

    def look(self) -> list[list[str]]:
        return self._look_tiles

    def inventory(self) -> dict[str, int]:
        return self._inventory

    def incantation(self) -> int | None:
        self.calls.append("Incantation")
        return self.incantation_result

    def pop_messages(self) -> list[tuple[int, str]]:
        messages = self._messages
        self._messages = []
        return messages


def check(label: str, condition: bool) -> bool:
    status = "PASS" if condition else "FAIL"
    print(f"[{status}] {label}")
    return condition


def scenario_survival() -> bool:
    client = FakeClient(
        look_tiles=[["food"], [], [], []],
        inventory={"food": 1, "linemate": 0},
    )
    strategy = Strategy(client, team="alpha", level=1)
    strategy.step()
    return check("low food takes the food on the current tile", client.calls == ["Take food"])


def scenario_incanting_alone() -> bool:
    client = FakeClient(
        look_tiles=[["player"], [], [], []],
        inventory={"food": 10, "linemate": 1},
    )
    client.incantation_result = 2
    strategy = Strategy(client, team="alpha", level=1)
    strategy.step()
    level_updated = strategy.level == 2
    return check(
        "enough food, stones, and players starts the incantation",
        "Incantation" in client.calls and level_updated,
    )


def scenario_calling_for_help() -> bool:
    client = FakeClient(
        look_tiles=[["player"], [], [], []],
        inventory={"food": 10, "linemate": 1, "deraumere": 1, "sibur": 1},
    )
    strategy = Strategy(client, team="alpha", level=2)
    strategy.step()
    return check(
        "ready to elevate but alone broadcasts a rally call",
        any(call.startswith("Broadcast 2|alpha|") for call in client.calls),
    )


def scenario_gathering_moves_to_resource() -> bool:
    client = FakeClient(
        look_tiles=[["player"], [], ["linemate"], []],
        inventory={"food": 5, "linemate": 0},
    )
    strategy = Strategy(client, team="alpha", level=1)
    strategy.step()
    return check("missing stone visible nearby triggers movement towards it", len(client.calls) > 0)


def scenario_consolidates_with_neighbors_already_here() -> bool:
    client = FakeClient(
        # 2 other players already on this tile (e.g. survivors of the
        # incantation that just leveled us up), food just under the gather
        # threshold and only one stone: too weak to trigger a broadcast,
        # but the opportunity costs nothing to check since nobody moves.
        look_tiles=[["player", "player", "player"], [], [], []],
        inventory={"food": 7, "linemate": 1, "deraumere": 0, "sibur": 0, "phiras": 0},
    )
    strategy = Strategy(client, team="alpha", level=4)
    strategy.step()
    return check(
        "already being grouped with others is exploited instead of wandering off",
        strategy.rallying and "Forward" not in client.calls,
    )


def scenario_survival_overrides_active_rally() -> bool:
    client = FakeClient(
        look_tiles=[["food"], [], [], []],
        inventory={"food": 1, "linemate": 1, "deraumere": 1, "sibur": 1},
    )
    strategy = Strategy(client, team="alpha", level=2)
    strategy.rallying = True
    strategy.ready_seen["ffffffff"] = {"direction": 4, "age": 1, "resources": {}}
    strategy.step()
    return check(
        "low food eats instead of following a stale rally",
        client.calls == ["Take food"] and not strategy.rallying,
    )


def scenario_stops_at_a_partial_group_instead_of_overshooting() -> bool:
    client = FakeClient(
        # 2 players already on this tile (self + one other), short of the
        # level-4 quorum of 4: joining must stop here, not walk past them.
        look_tiles=[["player", "player"], [], [], []],
        inventory={"food": 10, "linemate": 1, "deraumere": 1, "sibur": 2, "phiras": 1},
    )
    strategy = Strategy(client, team="alpha", level=4)
    strategy.rallying = True
    strategy.target_token = "bbbbbbbb"
    strategy.ready_seen["bbbbbbbb"] = {"direction": 4, "age": 0, "resources": {}}
    strategy.step()
    return check(
        "meeting only some of the required players stops the chase instead of overshooting",
        "Forward" not in client.calls and "Left" not in client.calls and "Right" not in client.calls,
    )


def scenario_high_level_pair_does_not_stop_the_rally() -> bool:
    client = FakeClient(
        look_tiles=[["player", "player"], [], [], []],
        inventory={"food": 20, "linemate": 1, "deraumere": 2, "sibur": 3, "phiras": 1},
    )
    strategy = Strategy(client, team="alpha", level=6)
    strategy.rallying = True
    strategy.target_token = "bbbbbbbb"
    strategy.ready_seen["bbbbbbbb"] = {"direction": 4, "age": 1, "resources": {}}
    strategy.step()
    return check(
        "a high-level rally does not camp on a mere pair of players",
        client.calls == ["Forward"],
    )


def scenario_high_level_cluster_keeps_stones_until_quorum() -> bool:
    client = FakeClient(
        look_tiles=[["player", "player", "player"], [], [], []],
        inventory={"food": 20, "linemate": 1, "deraumere": 2, "sibur": 0, "phiras": 0},
    )
    strategy = Strategy(client, team="alpha", level=6)
    strategy.step()
    return check(
        "a high-level partial cluster broadcasts but keeps stones in inventory",
        strategy.rallying
        and any(call.startswith("Broadcast 6|alpha|") for call in client.calls)
        and not any(call.startswith("Set ") for call in client.calls),
    )


def scenario_high_level_cluster_deposits_at_quorum() -> bool:
    client = FakeClient(
        look_tiles=[["player", "player", "player", "player", "player", "player"], [], [], []],
        inventory={"food": 20, "linemate": 1, "deraumere": 0, "sibur": 0, "phiras": 0},
    )
    strategy = Strategy(client, team="alpha", level=6)
    strategy.step()
    return check(
        "a high-level cluster starts depositing once the quorum is present",
        "Set linemate" in client.calls,
    )


def scenario_ready_ping_advertises_but_waits_for_quorum() -> bool:
    client = FakeClient(
        look_tiles=[["player"], [], [], []],
        inventory={"food": 10, "linemate": 1, "deraumere": 1, "sibur": 2, "phiras": 1},
    )
    strategy = Strategy(client, team="alpha", level=4)
    strategy.step()
    return check(
        "a lone ready level-4 player advertises readiness but does not converge yet",
        any(call.startswith("Broadcast 4|alpha|") for call in client.calls) and not strategy.rallying,
    )


def scenario_stale_bearing_only_goes_forward() -> bool:
    client = FakeClient(
        look_tiles=[["player"], [], [], []],
        inventory={"food": 10, "linemate": 1, "deraumere": 1, "sibur": 2, "phiras": 1},
    )
    strategy = Strategy(client, team="alpha", level=4)
    strategy.rallying = True
    strategy.target_token = "bbbbbbbb"
    # direction 2 means "Left, Forward" - only meaningful relative to our
    # facing at the moment it was heard, not something to redo every turn.
    client._messages.append((2, "4|alpha|bbbbbbbb|none"))
    strategy.step()
    fresh_calls = list(client.calls)
    client.calls.clear()
    # No new ping arrives this turn: the bearing learned last turn is stale.
    strategy.step()
    return check(
        "a bearing not refreshed this turn only moves forward, it does not re-turn",
        "Left" in fresh_calls and client.calls == ["Forward"],
    )


def scenario_pooled_resources_trigger_convergence() -> bool:
    client = FakeClient(
        look_tiles=[["player"], [], [], []],
        inventory={"food": 10, "linemate": 1, "deraumere": 0, "sibur": 0, "phiras": 0},
    )
    strategy = Strategy(client, team="alpha", level=4)
    strategy.ready_seen["bbbbbbbb"] = {
        "direction": 2,
        "age": 0,
        "resources": {"deraumere": 1, "sibur": 2, "phiras": 1},
    }
    strategy.ready_seen["cccccccc"] = {"direction": 6, "age": 0, "resources": {}}
    strategy.ready_seen["dddddddd"] = {"direction": 1, "age": 0, "resources": {}}
    strategy.step()
    return check(
        "enough ready players whose pooled stones cover the requirement start converging",
        strategy.rallying,
    )


def scenario_picks_up_stone_while_joining_without_detour() -> bool:
    client = FakeClient(
        look_tiles=[["player", "phiras"], [], [], []],
        inventory={"food": 10, "linemate": 1, "deraumere": 1, "sibur": 2, "phiras": 0},
    )
    strategy = Strategy(client, team="alpha", level=4)
    strategy.rallying = True
    strategy.target_token = "bbbbbbbb"
    strategy.ready_seen["bbbbbbbb"] = {"direction": 4, "age": 0, "resources": {}}
    strategy.step()
    return check(
        "a needed stone already on the current tile is picked up while joining",
        "Take phiras" in client.calls,
    )


def scenario_stays_locked_on_target_despite_fresher_pings() -> bool:
    client = FakeClient(
        look_tiles=[["player"], [], [], []],
        inventory={"food": 10, "linemate": 1, "deraumere": 0, "sibur": 0, "phiras": 0},
    )
    strategy = Strategy(client, team="alpha", level=4)
    strategy.ready_seen["bbbbbbbb"] = {
        "direction": 2,
        "age": 0,
        "resources": {"deraumere": 1, "sibur": 2, "phiras": 1},
    }
    strategy.ready_seen["cccccccc"] = {"direction": 6, "age": 0, "resources": {}}
    strategy.ready_seen["dddddddd"] = {"direction": 1, "age": 0, "resources": {}}
    strategy.step()
    locked_on = strategy.target_token
    # A brand new ping, fresher than everything else, arrives the next turn.
    client._messages.append((5, "4|alpha|eeeeeeee|linemate:1"))
    strategy.step()
    return check(
        "a fresher ping heard mid-rally does not steal the lock from the current target",
        strategy.target_token == locked_on,
    )


def scenario_gives_up_on_an_unreachable_rally() -> bool:
    client = FakeClient(
        look_tiles=[["player"], [], [], []],
        inventory={"food": 20, "linemate": 0, "deraumere": 0, "sibur": 0},
    )
    strategy = Strategy(client, team="alpha", level=2)
    strategy.rallying = True
    strategy.ready_seen["aaaaaaaa"] = {"direction": 4, "age": 0, "resources": {}}
    strategy.rally_turns = 60
    strategy.step()
    return check(
        "chasing an unreachable rally for too long gives it up instead of forever wandering",
        not strategy.rallying and len(strategy.ready_seen) == 0,
    )


def scenario_skips_fork_when_egg_already_waiting() -> bool:
    client = FakeClient(
        look_tiles=[["player"], [], [], []],
        inventory={"food": 20, "linemate": 0},
    )
    client.available_slots = 1
    strategy = Strategy(client, team="alpha", level=1)
    strategy.step()
    return check(
        "an unhatched egg already waiting stops a new fork",
        "Connect_nbr" in client.calls and "Fork" not in client.calls,
    )


def main() -> int:
    scenarios = [
        scenario_survival,
        scenario_incanting_alone,
        scenario_calling_for_help,
        scenario_gathering_moves_to_resource,
        scenario_survival_overrides_active_rally,
        scenario_consolidates_with_neighbors_already_here,
        scenario_stops_at_a_partial_group_instead_of_overshooting,
        scenario_high_level_pair_does_not_stop_the_rally,
        scenario_high_level_cluster_keeps_stones_until_quorum,
        scenario_high_level_cluster_deposits_at_quorum,
        scenario_stale_bearing_only_goes_forward,
        scenario_ready_ping_advertises_but_waits_for_quorum,
        scenario_pooled_resources_trigger_convergence,
        scenario_picks_up_stone_while_joining_without_detour,
        scenario_stays_locked_on_target_despite_fresher_pings,
        scenario_gives_up_on_an_unreachable_rally,
        scenario_skips_fork_when_egg_already_waiting,
    ]
    results = [scenario() for scenario in scenarios]
    if all(results):
        print("All scenarios passed.")
        return 0
    print("Some scenarios failed.")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
