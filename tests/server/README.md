# Server tests

Three categories of tests cover the Zappy server at different levels of granularity.

## Functional tests

Functional tests validate the observable behavior of the server as a whole: they launch a real server process, connect AI and GUI clients through the network, and verify that the protocol exchanges are correct end-to-end. They catch regressions that only appear when all subsystems (network loop, command parser, simulation) interact together, and they serve as the authoritative proof that the server satisfies the project specification.

## Stress tests

Stress tests measure the robustness of the server under abnormal load: mass simultaneous connections, command queue flooding, malformed input, rapid connect/disconnect storms, and high-frequency GUI/AI traffic. They expose resource leaks, deadlocks, file-descriptor exhaustion, and undefined behavior that only surface under sustained pressure — conditions that functional tests rarely reproduce.

## Unit tests

Unit tests verify individual components in isolation: command handlers, the command parser, the world simulation, player logic, and item helpers. By covering each class and function independently they make it easy to locate the exact source of a regression, and they provide a safety net when refactoring internals without changing the external protocol.
