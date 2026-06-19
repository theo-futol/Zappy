"""Fallback headless UI for running the lab server without pygame."""

from __future__ import annotations

import time


class HeadlessUI:

    def __init__(self, server) -> None:
        self.server = server
        self._last_tick = 0.0
        self._winner_announced = False

    def init(self) -> None:
        self._last_tick = time.perf_counter()
        print("Headless mode enabled: pygame UI is unavailable.")

    def handle_events(self) -> float:
        now = time.perf_counter()
        dt = now - self._last_tick
        self._last_tick = now
        return dt

    def render(self) -> None:
        if self.server.game_over and not self._winner_announced:
            self._winner_announced = True
            print(
                f"[server] WINNER {self.server.winner_team} "
                f"({self.server.winner_player_count} players reached level 8)"
            )
        return

    def quit(self) -> None:
        return
