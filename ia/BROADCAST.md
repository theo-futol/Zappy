sender: "level, intention, leader_token, resources"
receiver: direction, "level, intention, leader_token, resources"

example:
sender: "2, incantation_call, ab12cd34, 1 linemate 1 deraumere 1 sibur"

support examples:
sender: "2, incantation_available, ab12cd34, none"
sender: "2, incantation_arriving, ab12cd34, none"

notes:
- level is the current level of the player asking for help
- intention is used to decide whether another player should join
- `leader_token` identifies the leader call currently being followed
- `incantation_call` is the real rally signal to follow
- `incantation_available` means "I can help"
- `incantation_arriving` means "I am on the way"
- resources describe what is still missing when relevant
