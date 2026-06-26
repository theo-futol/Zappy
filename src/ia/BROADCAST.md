sender: "level|team|leader_token"
receiver: direction, "level|team|leader_token"

example:
sender: "4|alpha|ab12cd34"

notes:
- `level` is the current level of the player asking for help.
- This is the only broadcast emitted by the AI: a rally call for an incantation.
- `leader_token` identifies the call currently being followed; the lowest token
  among concurrent calls for the same level/team wins, so two ready players don't
  keep swapping leadership.
- A receiver only reacts if the message's team and level match its own, and it
  does not already have enough players on its own tile to incant alone.
- A rally call is forgotten after `RALLY_CALL_EXPIRY_TURNS` turns without a fresh
  matching broadcast.
