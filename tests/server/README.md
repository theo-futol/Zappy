# Server tests

Three categories of tests cover the Zappy server at different levels of granularity.

## Functional tests

Functional tests validate the observable behavior of the server as a whole: they launch a real server process, connect AI and GUI clients through the network, and verify that the protocol exchanges are correct end-to-end. They catch regressions that only appear when all subsystems (network loop, command parser, simulation) interact together, and they serve as the authoritative proof that the server satisfies the project specification.

## Stress tests

Stress tests measure the robustness of the server under abnormal load: mass simultaneous connections, command queue flooding, malformed input, rapid connect/disconnect storms, and high-frequency GUI/AI traffic. They expose resource leaks, deadlocks, file-descriptor exhaustion, and undefined behavior that only surface under sustained pressure — conditions that functional tests rarely reproduce.

## Unit tests

Unit tests verify individual components in isolation: command handlers, the command parser, the world simulation, player logic, and item helpers. By covering each class and function independently they make it easy to locate the exact source of a regression, and they provide a safety net when refactoring internals without changing the external protocol.

## Running tests

All tests are driven by a single entry point at the root of the repository:

```bash
./start_test.sh <unit:0|1> <stress:0|1> <functional:0|1>
```

Each flag independently enables the corresponding category. Examples:

| Command | What runs |
|---|---|
| `./start_test.sh 1 0 0` | unit tests only |
| `./start_test.sh 0 1 1` | stress + functional tests |
| `./start_test.sh 1 1 1` | everything |

### Why you do not need to start the server manually

`start_test.sh` handles the full lifecycle automatically:

1. **Compile** — it runs `make zappy_server` before any network test.
2. **Start** — it launches the server in the background with fixed parameters:
   ```
   ./zappy_server -c 5 -n team1 team2 -p 4242 -x 10 -y 10 -f 100
   ```
   This gives two teams (`team1`, `team2`), a 10 × 10 map, port 4242, and a frequency of 100 — the same defaults assumed by every stress and functional test script.
3. **Test** — it runs the selected test suites against that live server.
4. **Teardown** — it kills the server process once all tests have finished, regardless of outcome.

Unit tests do not require a server at all: they link directly against the compiled source objects and run entirely in-process.
