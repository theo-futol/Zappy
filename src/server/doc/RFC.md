# RFC

# Protocol

All commands are transmitted through a character string that ends with a new line. Elements between angle brackets are arguments to replace. In case of a bad/unknown command, the server must answer "ko".

| Commands | Responses | Example |
|---|---|---|
| `Forward`, `Right`, `Left` | Status | ok, ko / dead |
| `Look`, `Inventory` | [data]\n | [tile1, tile2…] |
| `Connect_nbr` | value\n | 2 |
| `Broadcast` \<text\> | Status | ok, ko / dead |
| `Eject` | Status | ok/ko |
| `Take` \<object\> | Status | ok/ko |
| `Set` \<object\> | Status | ok/ko |
| `Incantation` | Status | Elevation underway / Current level: k / ko |

However there is a special case which is the instance of a GUI instance connecting to the server. The following needs to be followed:

| Command | Direction | Response / Format | Description |
|---|---|---|---|
| `msz X Y` | Server | `msz` | Map size |
| `bct X Y q0 q1 q2 q3 q4 q5 q6` | Server | `bct X Y` | Content of a tile |
| `bct X Y q0 q1 q2 q3 q4 q5 q6` × nbr_tiles | Server | `mct` | Content of the map (all tiles) |
| `tna N` × nbr_teams | Server | `tna` | Name of all the teams |
| `pnw #n X Y O L N` | Server | — | Connection of a new player |
| `ppo #n X Y O` | Both | `ppo #n` | Player's position |
| `plv #n L` | Both | `plv #n` | Player's level |
| `pin #n X Y q0 q1 q2 q3 q4 q5 q6` | Both | `pin #n` | Player's inventory |
| `pex #n` | Server | — | Expulsion |e
| `pbc #n M` | Server | — | Broadcast |
| `pic X Y L #n #n ...` | Server | — | Start of an incantation (by the first player) |
| `pie X Y R` | Server | — | End of an incantation |
| `pfk #n` | Server | — | Egg laying by the player |
| `pdr #n i` | Server | — | Resource dropping |
| `pgt #n i` | Server | — | Resource collecting |
| `pdi #n` | Server | — | Death of a player |
| `enw #e #n X Y` | Server | — | An egg was laid by a player |
| `ebo #e` | Server | — | Player connection for an egg |
| `edi #e` | Server | — | Death of an egg |
| `sgt T` | Both | `sgt` | Time unit request |
| `sst T` | Both | `sst T` | Time unit modification |
| `seg N` | Server | — | End of game |
| `smg M` | Server | — | Message from the server |
| `suc` | Client | — | Unknown command |
| `sbp` | Client | — | Command parameter error |


# TO REMOVE
- poll -> need timeout don't forget, foutre un chrono dedans avant.
- player needs to have a isActive (If no eggs are in the teams, no spawn)
- map stocker vector vector vector ??? (see other commands such as Broadcast to verify it's usefulness)


vector<shared_ptr<Player>>

struct tile 
{
    vector<shared_ptr<Player>>
    vector<>

}






