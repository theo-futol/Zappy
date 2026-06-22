# Zappy Lab Environment - Project State

## Project Overview
The objective is to establish a local, deterministic, and highly observable lab environment ("Antigravity Server") mimicking the official Zappy network game server. This is a Python-based mock server designed for rapid prototyping, debugging, and regression testing of AI clients without needing the full C/SFML Zappy server stack.

## Current State

The initial project setup has been completed based on the specifications detailed in `instrcution.txt` and `subject.txt`.

### Implemented Components

1. **`config.json`**:
   - Contains the configuration for the server: dimensions (`width`, `height`), `port`, `teams`, `initial_clients_per_team`, and the time scaling factor `default_freq`.

2. **`server.py`**:
   - A skeleton implementation of the **Antigravity Server**.
   - **Concurrency Model**: Implemented using a single thread cooperative multitasking architecture. An `asyncio` event loop shares time with the `pygame` rendering engine. A `pygame.time.Clock().tick(60)` handles framerate, and `await asyncio.sleep(0.001)` yields CPU to process incoming TCP buffers.
   - **TCP Async Server**: Listens for AI clients connecting via TCP sockets. It responds with `WELCOME`, accepts `<TEAM-NAME>`, and currently mocks the `<CLIENT-NUM>` and `<X> <Y>` handshake responses.
   - **Decoupled Time Engine**: Calculates `virtual_dt` based on the real `dt` and `default_freq`, with a pause/resume toggle accessible via the `P` key.

### Pending Implementations for Future Agents

The foundational architecture is up, but the core game mechanics and networking protocols need to be fully fleshed out:

1. **World Mechanics & Geography**:
   - **Toroidal Map**: Movement calculations and distance functions (for the `broadcast` command) need to account for wrapping edges.
   - **Resource Management**: Implement the formula for resource density (Food, Linemate, Deraumere, Sibur, Mendiane, Phiras, Thystame) and resource regeneration every `20/f` simulated seconds.

2. **Client Command Processing (TCP Buffering)**:
   - AI clients can queue up to 10 requests. Commands beyond this buffer must be ignored.
   - Actions must take simulated time (`virtual_dt`) to execute (e.g., `Forward` costs `7/f`, `Incantation` costs `300/f`). This blocks only the acting player.
   - Implement action handlers for the full command set (`Forward`, `Right`, `Left`, `Look`, `Inventory`, `Broadcast`, `Connect_nbr`, `Fork`, `Eject`, `Take`, `Set`, `Incantation`).

3. **Lifecycle & Metabolism**:
   - Players consume 1 unit of food every `126/f` simulated seconds.
   - If food reaches 0, the server must send `dead\n`, close the TCP socket, and remove the player from the map.

4. **Elevation Mechanics**:
   - The `Incantation` command requires complex validation of players on the same tile, levels, and stones at both the start (tick 0) and the end (tick 300/f) of the ritual.

5. **Lab Inspection & UI Layout**:
   - Implement the `pygame` visual interface.
   - **Grid Rendering**: Render low-contrast bounding squares.
   - **Resource Visualization**: Small colored blocks with count text on tiles.
   - **Player Tokens**: Geometric triangles showing direction and team color.
   - **Diagnostic Panel**: Handle `pygame.mouse.get_pos()` to select a tile/player and output state details (Level, Position, Metabolism, Inventory, Command Queue, HUD).

## References
- `instrcution.txt`: Contains specific architecture rules for the Antigravity Python server.
- `subject.txt`: Contains general Zappy game rules, mechanics, resources, elevation tables, and commands.
