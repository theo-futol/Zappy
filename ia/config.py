"""Tunable parameters for the Zappy AI."""

# Default high-level objective used by the client.
DEFAULT_OBJECTIVE = "exploration"

# Below this amount of food, drop everything and go eat.
# Each unit lasts 126 ticks; vision is narrow (radius 1 at level 1), so the
# buffer needs to survive several turns of blind searching.
SURVIVAL_FOOD_THRESHOLD = 4

# Below this amount of food, prioritize food over stone gathering.
GATHER_FOOD_THRESHOLD = 8

# Minimum food before forking a new egg.
FORK_FOOD_THRESHOLD = 8

# Minimum turns between two forks.
FORK_COOLDOWN_TURNS = 30

# Minimum turns between two rally broadcasts.
BROADCAST_COOLDOWN_TURNS = 10

# A ready ping is forgotten after this many turns without a fresh one,
# so a stale/dead sender eventually drops out of the pooled tally.
RALLY_CALL_EXPIRY_TURNS = 15

# Give up on a rally (travelling or pooling on the spot) after this many
# turns: broadcasts carry no distance, so a converging group can turn out
# to be unreachable, or short of a resource nobody actually had.
RALLY_GIVE_UP_TURNS = 60

# Turn once after this many straight exploration moves to avoid infinite lines.
EXPLORE_TURN_INTERVAL = 4
