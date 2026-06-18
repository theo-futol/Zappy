"""Tunable parameters for the Zappy AI."""

# Default high-level objective used by the client and the model.
DEFAULT_OBJECTIVE = "exploration"

# Refresh the inventory after this many actions to limit local drift.
DEFAULT_INVENTORY_REFRESH_INTERVAL = 5

# Refresh the vision after this many vision-changing actions.
# Use 1 to keep the current immediate look behavior.
LOOK_REFRESH_INTERVAL = 1

# Keep an allied broadcast request active longer as the level increases.
ALLY_BROADCAST_BASE_MAX_AGE = 12
ALLY_BROADCAST_MAX_AGE_PER_LEVEL = 4
OUTGOING_BROADCAST_BASE_REPEAT_INTERVAL = 5
OUTGOING_BROADCAST_REPEAT_INTERVAL_PER_LEVEL = 1

# Avoid dropping stones one by one before the full incantation set is ready.
PLACE_STONES_ONLY_WHEN_INVENTORY_READY = True

# Food thresholds.
SURVIVAL_FOOD_THRESHOLD = 10
OPPORTUNISTIC_FOOD_THRESHOLD = 18
FORK_FOOD_THRESHOLD = 18
ALLY_HELP_FOOD_THRESHOLD = 20
INCANTATION_FOOD_THRESHOLD = 10

# Tile conditions.
OVERCROWD_EJECT_THRESHOLD = 3

# Cooldowns are counted in completed commands.
ACTION_COOLDOWN_STEPS = {
    "Broadcast": 4,
    "Fork": 12,
    "Eject": 8,
}

# Force a deterministic extra action after this many safe turns.
FORCE_OPPORTUNISTIC_AFTER_SAFE_TURNS = 6
FORK_SAFE_TURNS_THRESHOLD = 3
WAIT_INCANTATION_FORK_BASE_TURNS = 3
WAIT_INCANTATION_FORK_TURNS_PER_LEVEL = 1

# Turn once after this many exploration decisions to avoid infinite straight lines.
EXPLORE_TURN_INTERVAL = 4


def get_ally_broadcast_max_age(level: int) -> int:
    return _get_level_scaled_value(
        ALLY_BROADCAST_BASE_MAX_AGE,
        ALLY_BROADCAST_MAX_AGE_PER_LEVEL,
        level,
    )


def get_outgoing_broadcast_repeat_interval(level: int) -> int:
    return _get_level_scaled_value(
        OUTGOING_BROADCAST_BASE_REPEAT_INTERVAL,
        OUTGOING_BROADCAST_REPEAT_INTERVAL_PER_LEVEL,
        level,
    )


def get_wait_incantation_fork_turns(level: int) -> int:
    return _get_level_scaled_value(
        WAIT_INCANTATION_FORK_BASE_TURNS,
        WAIT_INCANTATION_FORK_TURNS_PER_LEVEL,
        level,
    )


def _get_level_scaled_value(base: int, per_level: int, level: int) -> int:
    normalized_level = max(1, int(level))
    return int(base) + (normalized_level - 1) * int(per_level)
