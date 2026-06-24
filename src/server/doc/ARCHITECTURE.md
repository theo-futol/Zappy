# ZAPPY SERVER ARCHITECTURE

The purpose of this document is to provide a high-level overview of the architecture of the Zappy server. It outlines the main components, their interactions, and the overall design principles that guide the development of the server.

# Tree Structure

```
src/server
├── ArgParser
├── Core
├── Network
│   ├── Client
│   ├── ClientHandler
│   │   ├── CommandParser
│   │   └── Commands
│   │       ├── AI
│   │       └── GUI
│   └── Socket
├── ServerException
└── Simulation
    ├── Player
    │   └── Inventory
    └── World
```

## Component descriptions

- **[ArgParser](ArgParser.md)**: command-line argument parser that handles user inputs and configurations. Responsible for parsing and validating the arguments provided by the user.

- **[Core](Core.md)**: core functionality of the application, including the main logic and processing of data. Handles signals, manages flags, and orchestrates the overall flow of the application.

- **[Network/Client](Client.md)**: client-side networking components, covering both AI and GRAPHIC client types. Handles communication through sockets (via file descriptors) and manages the input buffer for each client.

- **[Network/ClientHandler](ClientHandler.md)**: handles the processing of incoming commands from clients, including parsing and executing commands (delegated to the CommandParser).

- **[Network/ClientHandler/CommandParser](CommandParser.md)**: responsible for parsing and validating commands received from clients, ensuring they adhere to the expected format and semantics. Also stores all the commands handled by the server, for both GUI and AI clients.

- **[Network/ClientHandler/Commands](Commands.md)**: individual command implementations defining the specific actions taken in response to client commands. Each command is encapsulated inside a Command class, split between `AI` (player commands) and `GUI` (graphical client commands).

- **[Network/ClientHandler/ClientHandler](ClientHandler.md)**: manages the overall client connection, including reading from and writing to the socket, maintaining client state, and coordinating with the CommandParser to process incoming commands. Also handles the connection lifecycle (initialization, cleanup, error handling) using poll, including broadcasting to players, broadcasting to GUI clients, and resource regeneration.

- **[Network/Socket](Socket.md)**: encapsulation of the socket communication used to construct the server.

- **[ServerException](ServerException.md)**: custom exception class used across the server.

- **[Simulation](Simulation.md)**: the simulation environment, including the game world (owning the players and items on each tile), players (including inventory, movement, and level), and items.

- **[Utils](Utils.md)**: utility functions and structures providing common functionality used throughout the application, such as position calculations, direction handling, and other helper methods.
