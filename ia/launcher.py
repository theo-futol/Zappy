"""Supervise multiple Zappy AI clients for ai_lab."""

from __future__ import annotations

import argparse
import asyncio
import json
import re
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_SERVER_CONFIG = REPO_ROOT / "ai_lab" / "config.json"
DEFAULT_MIN_TARGET_PER_TEAM = 64
DEFAULT_TARGET_MULTIPLIER = 16
UNBOUNDED_TARGET_PER_TEAM = 10**9
DEFAULT_RETRY_INTERVAL = 3.0
DEFAULT_NO_SLOT_RETRY_INTERVAL = 15.0
DEFAULT_MAX_NO_SLOT_RETRY_INTERVAL = 60.0
DEFAULT_STATUS_INTERVAL = 5.0
DEFAULT_SPAWN_INTERVAL = 0.35
DEFAULT_POLL_INTERVAL = 0.25
GAME_OVER_EXIT_CODE = 20
CLIENT_DEATH_RE = re.compile(r"\bdead\s+level=(\d+)(?:\s+food=(-?\d+))?\b")
NO_SLOT_RE = re.compile(r"\bEquipe refusee par le serveur\b")


@dataclass
class ManagedClient:
    team: str
    instance_id: int
    process: asyncio.subprocess.Process
    spawned_at: float


@dataclass
class TeamRuntime:
    team: str
    target_count: int
    active_clients: dict[int, ManagedClient] = field(default_factory=dict)
    next_spawn_at: float = 0.0
    next_instance_id: int = 1
    no_slot_failures: int = 0


class ClientLauncher:

    def __init__(self, args: argparse.Namespace) -> None:
        self.args = args
        self.repo_root = REPO_ROOT
        self.server_config = load_server_config(Path(args.server_config))
        self.host = args.host
        self.port = args.port if args.port is not None else int(self.server_config["port"])
        self.teams = args.teams or list(self.server_config["teams"])
        self.target_per_team = resolve_target_per_team(
            requested_target=args.target_per_team,
            initial_clients_per_team=int(self.server_config["initial_clients_per_team"]),
        )
        self.retry_interval = max(0.2, float(args.retry_interval))
        self.no_slot_retry_interval = max(
            self.retry_interval,
            float(args.no_slot_retry_interval),
        )
        self.max_no_slot_retry_interval = max(
            self.no_slot_retry_interval,
            float(args.max_no_slot_retry_interval),
        )
        self.status_interval = max(1.0, float(args.status_interval))
        self.spawn_interval = max(0.05, float(args.spawn_interval))
        self.poll_interval = max(0.05, float(args.poll_interval))
        self._stopping = False
        self._started_at = time.monotonic()
        self._watch_tasks: set[asyncio.Task[None]] = set()
        self._team_runtimes = {
            team: TeamRuntime(team=team, target_count=self.target_per_team)
            for team in self.teams
        }

    async def run(self) -> None:
        self._started_at = time.monotonic()
        self._log(
            "launcher started "
            f"host={self.host} port={self.port} "
            f"teams={','.join(self.teams)} target_per_team={format_target(self.target_per_team)}"
        )

        last_status_at = 0.0
        loop = asyncio.get_running_loop()

        try:
            while not self._stopping:
                now = loop.time()
                await self._maybe_spawn_clients(now)

                if now - last_status_at >= self.status_interval:
                    self._log(self._build_status_line())
                    last_status_at = now

                await asyncio.sleep(self.poll_interval)
        finally:
            await self._stop_all_clients()

    async def _maybe_spawn_clients(self, now: float) -> None:
        for runtime in self._team_runtimes.values():
            if len(runtime.active_clients) >= runtime.target_count:
                continue
            if now < runtime.next_spawn_at:
                continue

            await self._spawn_one_client(runtime, now)
            runtime.next_spawn_at = now + self.spawn_interval

    async def _spawn_one_client(self, runtime: TeamRuntime, now: float) -> None:
        instance_id = runtime.next_instance_id
        runtime.next_instance_id += 1
        command = self._build_client_command(runtime.team)

        stdout_setting: int | None = None
        stderr_setting: int | None = None
        if not self.args.show_client_logs:
            stdout_setting = asyncio.subprocess.DEVNULL
            stderr_setting = asyncio.subprocess.PIPE

        process = await asyncio.create_subprocess_exec(
            *command,
            cwd=str(self.repo_root),
            stdout=stdout_setting,
            stderr=stderr_setting,
        )

        managed_client = ManagedClient(
            team=runtime.team,
            instance_id=instance_id,
            process=process,
            spawned_at=now,
        )
        runtime.active_clients[instance_id] = managed_client
        self._log(
            f"spawned {runtime.team}#{instance_id} pid={process.pid} "
            f"active={len(runtime.active_clients)}/{runtime.target_count}"
        )

        watch_task = asyncio.create_task(self._watch_client(runtime, managed_client))
        self._watch_tasks.add(watch_task)
        watch_task.add_done_callback(self._watch_tasks.discard)

    async def _watch_client(
        self,
        runtime: TeamRuntime,
        managed_client: ManagedClient,
    ) -> None:
        stderr_text = ""
        if managed_client.process.stderr is not None:
            stderr_output = await managed_client.process.stderr.read()
            stderr_text = stderr_output.decode("utf-8", errors="replace").strip()

        return_code = await managed_client.process.wait()
        runtime.active_clients.pop(managed_client.instance_id, None)
        now = asyncio.get_running_loop().time()
        age = max(0.0, now - managed_client.spawned_at)

        if self._stopping:
            return

        if return_code == GAME_OVER_EXIT_CODE:
            self._log(
                f"game over received from {runtime.team}#{managed_client.instance_id}, "
                "stopping launcher"
            )
            self._stopping = True
            return

        reason = self._summarize_stderr(stderr_text)
        death_summary = self._summarize_death(stderr_text)

        if self._is_no_slot_error(stderr_text):
            retry_delay = self._register_no_slot_failure(runtime)
            runtime.next_spawn_at = max(runtime.next_spawn_at, now + retry_delay)
            self._log(
                f"no slot {runtime.team}#{managed_client.instance_id} "
                f"retry_in={retry_delay:.1f}s reason={reason} "
                f"active={len(runtime.active_clients)}/{runtime.target_count}"
            )
            return

        runtime.no_slot_failures = 0
        runtime.next_spawn_at = max(runtime.next_spawn_at, now + self.retry_interval)
        if death_summary is not None:
            self._log(
                f"died {runtime.team}#{managed_client.instance_id} "
                f"{death_summary} age={age:.1f}s "
                f"active={len(runtime.active_clients)}/{runtime.target_count}"
            )
        self._log(
            f"stopped {runtime.team}#{managed_client.instance_id} "
            f"code={return_code} age={age:.1f}s reason={reason} "
            f"active={len(runtime.active_clients)}/{runtime.target_count}"
        )

    async def _stop_all_clients(self) -> None:
        self._stopping = True
        active_processes: list[asyncio.subprocess.Process] = []
        for runtime in self._team_runtimes.values():
            for managed_client in runtime.active_clients.values():
                active_processes.append(managed_client.process)

        if not active_processes:
            return

        self._log(f"stopping {len(active_processes)} client process(es)")

        for process in active_processes:
            if process.returncode is None:
                process.terminate()

        await asyncio.sleep(0.5)

        for process in active_processes:
            if process.returncode is None:
                process.kill()

        await asyncio.gather(*(process.wait() for process in active_processes), return_exceptions=True)
        await asyncio.gather(*self._watch_tasks, return_exceptions=True)

    def _build_client_command(self, team_name: str) -> list[str]:
        command = [
            self.args.python,
            "-m",
            "ia.client",
            "--host",
            self.host,
            "--port",
            str(self.port),
            "--team",
            team_name,
            "--objective",
            self.args.objective,
            "--inventory-refresh",
            str(self.args.inventory_refresh),
        ]

        if self.args.max_actions is not None:
            command.extend(["--max-actions", str(self.args.max_actions)])
        if not self.args.show_client_logs:
            command.append("--quiet")
        return command

    def _build_status_line(self) -> str:
        parts = []
        for runtime in self._team_runtimes.values():
            parts.append(
                f"{runtime.team}={len(runtime.active_clients)}/{format_target(runtime.target_count)}"
            )
        return "status " + " ".join(parts)

    def _summarize_stderr(self, stderr_text: str) -> str:
        if not stderr_text:
            return "no stderr"
        return stderr_text.splitlines()[-1]

    def _summarize_death(self, stderr_text: str) -> str | None:
        death_match = CLIENT_DEATH_RE.search(stderr_text)
        if death_match is None:
            return None
        level = death_match.group(1)
        food = death_match.group(2)
        if food is None:
            return f"level={level}"
        return f"level={level} food={food}"

    def _is_no_slot_error(self, stderr_text: str) -> bool:
        return NO_SLOT_RE.search(stderr_text) is not None

    def _register_no_slot_failure(self, runtime: TeamRuntime) -> float:
        retry_delay = min(
            self.max_no_slot_retry_interval,
            self.no_slot_retry_interval * (2 ** runtime.no_slot_failures),
        )
        runtime.no_slot_failures += 1
        return retry_delay

    def _log(self, message: str) -> None:
        elapsed = time.monotonic() - self._started_at
        print(f"[launcher t={elapsed:.1f}s] {message}")


def load_server_config(config_path: Path) -> dict[str, object]:
    with config_path.open("r", encoding="utf-8") as config_file:
        return json.load(config_file)


def resolve_target_per_team(
    *,
    requested_target: int | None,
    initial_clients_per_team: int,
) -> int:
    if requested_target == 0:
        return UNBOUNDED_TARGET_PER_TEAM
    if requested_target is not None:
        return max(1, int(requested_target))
    return max(
        DEFAULT_MIN_TARGET_PER_TEAM,
        int(initial_clients_per_team) * DEFAULT_TARGET_MULTIPLIER,
    )


def format_target(target_per_team: int) -> str:
    if target_per_team >= UNBOUNDED_TARGET_PER_TEAM:
        return "unbounded"
    return str(target_per_team)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Launch and supervise multiple Zappy AI clients.")
    parser.add_argument(
        "--server-config",
        default=str(DEFAULT_SERVER_CONFIG),
        help="Path to ai_lab config.json.",
    )
    parser.add_argument("--host", default="127.0.0.1", help="Server host.")
    parser.add_argument(
        "--port",
        type=int,
        default=None,
        help="Server port. Defaults to the value found in the server config.",
    )
    parser.add_argument(
        "--teams",
        nargs="*",
        default=None,
        help="Team names to supervise. Defaults to every team in the server config.",
    )
    parser.add_argument(
        "--target-per-team",
        type=int,
        default=None,
        help=(
            "Desired number of running clients per team. "
            "Use 0 to keep trying to fill every future slot. "
            "Defaults to a large headroom above the initial slots."
        ),
    )
    parser.add_argument(
        "--objective",
        default="exploration",
        help="Objective passed to each AI client.",
    )
    parser.add_argument(
        "--inventory-refresh",
        type=int,
        default=5,
        help="Inventory refresh interval passed to each AI client.",
    )
    parser.add_argument(
        "--max-actions",
        type=int,
        default=None,
        help="Optional max actions passed to each AI client.",
    )
    parser.add_argument(
        "--retry-interval",
        type=float,
        default=DEFAULT_RETRY_INTERVAL,
        help="Seconds to wait before retrying after a client exits.",
    )
    parser.add_argument(
        "--no-slot-retry-interval",
        type=float,
        default=DEFAULT_NO_SLOT_RETRY_INTERVAL,
        help="Initial seconds to wait after the server refuses a team because no slot is available.",
    )
    parser.add_argument(
        "--max-no-slot-retry-interval",
        type=float,
        default=DEFAULT_MAX_NO_SLOT_RETRY_INTERVAL,
        help="Maximum backoff after repeated no-slot refusals.",
    )
    parser.add_argument(
        "--spawn-interval",
        type=float,
        default=DEFAULT_SPAWN_INTERVAL,
        help="Minimum delay between spawn attempts for the same team.",
    )
    parser.add_argument(
        "--poll-interval",
        type=float,
        default=DEFAULT_POLL_INTERVAL,
        help="Supervisor loop sleep duration.",
    )
    parser.add_argument(
        "--status-interval",
        type=float,
        default=DEFAULT_STATUS_INTERVAL,
        help="Seconds between launcher status lines.",
    )
    parser.add_argument(
        "--python",
        default=sys.executable,
        help="Python executable used to start each client.",
    )
    parser.add_argument(
        "--show-client-logs",
        action="store_true",
        help="Keep client stdout/stderr attached to the terminal.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    launcher = ClientLauncher(args)
    try:
        asyncio.run(launcher.run())
    except KeyboardInterrupt:
        print("[launcher] interrupted")
        return 130
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
