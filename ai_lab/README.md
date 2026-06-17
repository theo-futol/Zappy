# Zappy Antigravity Server

The Antigravity Server is a local, deterministic, and highly observable lab environment designed to mimic the official Zappy network game server. It allows for rapid prototyping, debugging, and testing of Zappy AI clients in a Python-based ecosystem.

## Setup and Installation

1. **Install Dependencies**
   Install the graphics dependency that matches your Python version:
   ```bash
   pip install -r requirements.txt
   ```
   On Python 3.13 and older, this installs `pygame==2.6.1`.
   On Python 3.14 and newer, this installs `pygame-ce==2.5.7`, because it provides wheels for Python 3.14 on macOS while upstream `pygame` currently does not.

2. **Configuration**
   Before running the server, you can modify the `config.json` file to adjust the game parameters:
   - `port`: The TCP port the server listens on for AI clients.
   - `width` / `height`: Dimensions of the toroidal map.
   - `teams`: A list of team names allowed to connect.
   - `initial_clients_per_team`: The number of initial eggs/slots per team.
   - `default_freq`: The time reciprocal $f$ used for action execution costs.

## Starting the Server

Run the server module from the root directory:
```bash
python server.py
```
This single command starts the Asyncio TCP server. If the graphics dependency is installed, the graphical UI opens as well. Otherwise the server runs in headless mode.

## Interacting with the Server

### 1. Visual Interface (UI)
The graphical interface provides full visibility into the map and players:
- **Pause/Resume**: Press the `P` key on your keyboard to pause or resume the simulation. While paused, network commands are buffered but internal clocks freeze.
- **Inspect Entities**: Use your mouse to left-click any tile on the grid. A diagnostic side panel will open on the right displaying the contents of the tile (resources) and the full context of any players on that tile (level, food, starvation timer, inventory).

### 2. AI Client Connection
AI clients communicate via raw TCP sockets. You can manually test this by using `netcat` or `telnet`. 

In a separate terminal, connect to the server:
```bash
nc localhost 4242
```

**Connection Handshake Example:**
```text
<-- WELCOME
--> Team1
<-- 2          # (Number of available team slots remaining)
<-- 10 10      # (Map X and Y dimensions)
```

Once connected, you can type Zappy protocol commands (e.g., `Forward`, `Look`, `Inventory`, `Incantation`) followed by a newline, and the server will execute them based on their respective timing costs.
