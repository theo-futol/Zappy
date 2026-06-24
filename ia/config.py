"""Tunable parameters for the Zappy AI."""

# Default high-level objective used by the client and the model.
DEFAULT_OBJECTIVE = "exploration"

# Refresh the inventory after this many actions to limit local drift.
DEFAULT_INVENTORY_REFRESH_INTERVAL = 5

# Refresh the vision after this many vision-changing actions.
# Use 1 to keep the current immediate look behavior.
LOOK_REFRESH_INTERVAL = 1
HELP_INCANTATION_LOOK_REFRESH_INTERVAL = 2

# Keep an allied broadcast request active longer as the level increases.
ALLY_BROADCAST_BASE_MAX_AGE = 12
ALLY_BROADCAST_MAX_AGE_PER_LEVEL = 4
HIGH_LEVEL_COORDINATION_MIN_LEVEL = 6
HIGH_LEVEL_ALLY_BROADCAST_MAX_AGE = 24
ALLY_BROADCAST_SWITCH_MIN_AGE = 4
HIGH_LEVEL_ALLY_BROADCAST_SWITCH_MIN_AGE = 2
OUTGOING_BROADCAST_BASE_REPEAT_INTERVAL = 14
OUTGOING_BROADCAST_REPEAT_INTERVAL_PER_LEVEL = 2
INCANTATION_CALL_BASE_REPEAT_INTERVAL = 24
INCANTATION_CALL_REPEAT_INTERVAL_PER_LEVEL = 2
HIGH_LEVEL_INCANTATION_CALL_REPEAT_INTERVAL = 32
INCANTATION_AVAILABLE_REPEAT_INTERVAL = 14
INCANTATION_ARRIVING_REPEAT_INTERVAL = 10
REPEAT_INCANTATION_SUPPORT_BROADCASTS = False

# Avoid dropping stones one by one before the full incantation set is ready.
PLACE_STONES_ONLY_WHEN_INVENTORY_READY = True

# Food thresholds.
SURVIVAL_FOOD_THRESHOLD = 14
OPPORTUNISTIC_FOOD_THRESHOLD = 24
FORK_FOOD_THRESHOLD = 28
ALLY_HELP_FOOD_THRESHOLD = 20
LOW_LEVEL_HELP_FOOD_THRESHOLDS = {
    1: 8,
    2: 14,
    3: 18,
    4: 22,
}
HIGH_LEVEL_HELP_JOIN_FOOD_THRESHOLD = 45
HIGH_LEVEL_HELP_CANCEL_FOOD_THRESHOLD = 30
INCANTATION_FOOD_THRESHOLD = 10
GATHER_FOOD_THRESHOLD_BASE = 14
GATHER_FOOD_THRESHOLD_PER_LEVEL = 5
WAIT_INCANTATION_FOOD_THRESHOLD_BASE = 16
WAIT_INCANTATION_FOOD_THRESHOLD_PER_LEVEL = 5
PREPARE_INCANTATION_FOOD_THRESHOLD_BASE = 14
PREPARE_INCANTATION_FOOD_THRESHOLD_PER_LEVEL = 4
HIGH_LEVEL_GATHER_FOOD_THRESHOLD = 50
HIGH_LEVEL_WAIT_INCANTATION_FOOD_THRESHOLD = 50
HIGH_LEVEL_PREPARE_INCANTATION_FOOD_THRESHOLD = 35

# Tile conditions.
OVERCROWD_EJECT_THRESHOLD = 3

# Cooldowns are counted in completed commands.
ACTION_COOLDOWN_STEPS = {
    "Broadcast": 8,
    "Fork": 12,
    "Eject": 8,
}

# Force a deterministic extra action after this many safe turns.
FORCE_OPPORTUNISTIC_AFTER_SAFE_TURNS = 6
FORK_SAFE_TURNS_THRESHOLD = 3
WAIT_INCANTATION_FORK_BASE_TURNS = 3
WAIT_INCANTATION_FORK_TURNS_PER_LEVEL = 1
HIGH_LEVEL_WAIT_INCANTATION_FORK_BASE_TURNS = 14
HIGH_LEVEL_WAIT_INCANTATION_FORK_TURNS_PER_LEVEL = 4
HIGH_LEVEL_DISABLE_WAIT_FORK = False

# Turn once after this many exploration decisions to avoid infinite straight lines.
EXPLORE_TURN_INTERVAL = 4


def get_ally_broadcast_max_age(level: int) -> int:
    if is_high_level_coordination(level):
        return int(HIGH_LEVEL_ALLY_BROADCAST_MAX_AGE)
    max_age = _get_level_scaled_value(
        ALLY_BROADCAST_BASE_MAX_AGE,
        ALLY_BROADCAST_MAX_AGE_PER_LEVEL,
        level,
    )
    return max_age


def get_outgoing_broadcast_repeat_interval(level: int) -> int:
    return _get_level_scaled_value(
        OUTGOING_BROADCAST_BASE_REPEAT_INTERVAL,
        OUTGOING_BROADCAST_REPEAT_INTERVAL_PER_LEVEL,
        level,
    )


def get_ally_broadcast_switch_min_age(level: int) -> int:
    if is_high_level_coordination(level):
        return int(HIGH_LEVEL_ALLY_BROADCAST_SWITCH_MIN_AGE)
    return int(ALLY_BROADCAST_SWITCH_MIN_AGE)


def get_incantation_call_repeat_interval(level: int) -> int:
    if is_high_level_coordination(level):
        return int(HIGH_LEVEL_INCANTATION_CALL_REPEAT_INTERVAL)
    return _get_level_scaled_value(
        INCANTATION_CALL_BASE_REPEAT_INTERVAL,
        INCANTATION_CALL_REPEAT_INTERVAL_PER_LEVEL,
        level,
    )


def get_wait_incantation_fork_turns(level: int) -> int:
    if is_high_level_coordination(level):
        return _get_high_level_scaled_value(
            HIGH_LEVEL_WAIT_INCANTATION_FORK_BASE_TURNS,
            HIGH_LEVEL_WAIT_INCANTATION_FORK_TURNS_PER_LEVEL,
            level,
        )
    return _get_level_scaled_value(
        WAIT_INCANTATION_FORK_BASE_TURNS,
        WAIT_INCANTATION_FORK_TURNS_PER_LEVEL,
        level,
    )


def get_ally_help_food_threshold(level: int, *, committed: bool = False) -> int:
    if not is_high_level_coordination(level):
        threshold = int(
            LOW_LEVEL_HELP_FOOD_THRESHOLDS.get(
                max(1, int(level)),
                ALLY_HELP_FOOD_THRESHOLD,
            )
        )
        if committed:
            return max(int(INCANTATION_FOOD_THRESHOLD), threshold - 4)
        return threshold
    if committed:
        return int(HIGH_LEVEL_HELP_CANCEL_FOOD_THRESHOLD)
    return int(HIGH_LEVEL_HELP_JOIN_FOOD_THRESHOLD)


def get_gather_food_threshold(level: int) -> int:
    if is_high_level_coordination(level):
        return int(HIGH_LEVEL_GATHER_FOOD_THRESHOLD)
    return _get_level_scaled_value(
        GATHER_FOOD_THRESHOLD_BASE,
        GATHER_FOOD_THRESHOLD_PER_LEVEL,
        level,
    )


def get_wait_incantation_food_threshold(level: int) -> int:
    if is_high_level_coordination(level):
        return int(HIGH_LEVEL_WAIT_INCANTATION_FOOD_THRESHOLD)
    return _get_level_scaled_value(
        WAIT_INCANTATION_FOOD_THRESHOLD_BASE,
        WAIT_INCANTATION_FOOD_THRESHOLD_PER_LEVEL,
        level,
    )


def get_prepare_incantation_food_threshold(level: int) -> int:
    if is_high_level_coordination(level):
        return int(HIGH_LEVEL_PREPARE_INCANTATION_FOOD_THRESHOLD)
    return _get_level_scaled_value(
        PREPARE_INCANTATION_FOOD_THRESHOLD_BASE,
        PREPARE_INCANTATION_FOOD_THRESHOLD_PER_LEVEL,
        level,
    )


def is_high_level_coordination(level: int) -> bool:
    return max(1, int(level)) >= int(HIGH_LEVEL_COORDINATION_MIN_LEVEL)


def _get_level_scaled_value(base: int, per_level: int, level: int) -> int:
    normalized_level = max(1, int(level))
    return int(base) + (normalized_level - 1) * int(per_level)


def _get_high_level_scaled_value(base: int, per_level: int, level: int) -> int:
    normalized_level = max(int(HIGH_LEVEL_COORDINATION_MIN_LEVEL), int(level))
    return int(base) + (
        normalized_level - int(HIGH_LEVEL_COORDINATION_MIN_LEVEL)
    ) * int(per_level)
