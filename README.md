# Zappy

## Behaviour & Purpose

Zappy is **Epitech's PGE project** (Practical/Generic Engineering project), the
end-of-year project of the second year of the Epitech curriculum.

The goal of the project is to build a network survival game, *Trantor*, in
which several teams of autonomous AI clients compete on a shared, wrap-around
tile map scattered with food and mineral resources. Players (called
*Trantorians*) must feed themselves and gather stones in order to perform
*incantation* rituals and rise in elevation level. The first team to bring at
least six of its players to the maximum elevation level wins the game.

The project is split into three independent binaries, each with its own
language constraint:

- **zappy_server**: written in C++, generates and runs the world,
  enforces the rules of the game, and exposes the network protocol used by
  both AI and graphical clients.
- **zappy_gui**: written in OpenGL, a graphical, read-only client that connects
  to the server to observe and render the state of the world in real time.
- **zappy_ai**: written in a Python, an autonomous client that
  drives a single player by sending it commands over the network, with no
  further input from the user once launched.

The full protocol exchanged between the server and its clients (AI and GUI)
is detailed in [`rfc.txt`](./rfc.txt).

## Architecture Tree

```
Zappy/
├── src/
│   ├── main.cpp                  # Entry point of the zappy_server binary
│   └── server/
│       ├── Core/                 # Bootstraps the server: argument parsing, signal handling
│       ├── ArgParser/            # Registers and parses the CLI flags (-p, -x, -y, -n, -c, -f)
│       ├── ServerException/      # Server-specific exception types
│       ├── Network/
│       │   ├── Socket/           # Transport layer: accepts connections, reads/writes raw data
│       │   ├── ClientHandler/    # Retrieves client input via `poll`, queues it per client
│       │   │   ├── CommandParser/ # Parses queued input and dispatches to the right Commands
│       │   │   └── Commands/      # One implementation per protocol command (Look, Take, ...)
│       │   └── Client/            # Represents a connected client (AI or GUI) and its state
│       ├── Simulation/
│       │   ├── World/             # Map, tiles, resource spawning rules
│       │   └── Player/            # Player state: position, level, inventory, eggs
│       └── doc/                  # Server-side design notes (RFC draft)
├── rfc.txt                       # Full protocol/architecture RFC (server, AI, GUI)
├── Makefile                       # Top-level build rules (zappy_server, zappy_gui, zappy_ai)
└── dev/                            # Contributor tooling (branching/commit conventions)
```

- **GUI** (`zappy_gui`): *(to be completed)*
- **AI** (`zappy_ai`): *(to be completed)*

## Features

*(to be completed)*

## Dependencies & Installation

### Dependencies

*(to be completed)*

### Installation

Clone the repository:

```bash
git clone git@github.com:theo-futol/Zappy.git
cd Zappy
```

## Usage

### Demo

*(video to be added here)*

### Starting the project

Each binary is started independently and communicates over TCP, so the
server must be running before either client connects to it.

#### Server

```bash
./zappy_server -p port -x width -y height -n name1 name2 ... -c clientsNb -f freq
```

| Option | Description |
|---|---|
| `-p port` | Port number the server listens on |
| `-x width` | Width of the world |
| `-y height` | Height of the world |
| `-n name1 name2 ...` | Names of the teams |
| `-c clientsNb` | Number of initial client slots per team |
| `-f freq` | Reciprocal of the time unit used for executing actions |

Example:

```bash
./zappy_server -p 4242 -x 10 -y 10 -n team1 team2 -c 3 -f 100
```

#### GUI

*(to be completed)*

#### AI

*(to be completed)*

## Authors

- [Manny Bray-Mahinc](https://github.com/Man-epi)
- [Lucien Rivière](https://github.com/Say-Goodbi)
- [Quentin Taranne-Payet](https://github.com/Quentin-taranne)
- [Nathaniel Leperlier](https://github.com/leperlier-nathaniel)
- [Théo Futol](https://github.com/theo-futol)
- [Arthur Girardin-Calbe](https://github.com/arthur-girardin-calbe)
