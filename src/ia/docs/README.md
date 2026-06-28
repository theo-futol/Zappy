# Zappy AI Documentation

This documentation covers the Python AI client in `ia/`.

- [User guide](USER_GUIDE.md): running the AI, using the launcher, reading logs, and monitoring a game.
- [Developer guide](DEVELOPER_GUIDE.md): architecture, module contracts, protocol handling, tests, and diagnostics.
- [FSM details](FSM.md): states, transitions, incantation coordination, and the reasons behind the main safeguards.

Quick validation before a game:

```bash
python3 -m py_compile ia/*.py ia/utils/*.py
python3 ia/test.py
```
