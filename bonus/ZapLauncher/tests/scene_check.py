"""Run a playbook scene through the API and report pass/fail (exit code).

Used by bonus/run_tests.sh:  python tests/scene_check.py <playbook-name>
Needs ZAPPY_SERVER_HOST/ZAPPY_SERVER_PORT pointing at a running server.
"""

import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from fastapi.testclient import TestClient

import api


def main() -> int:
    name = sys.argv[1] if len(sys.argv) > 1 else "village-gathering"
    client = TestClient(api.app)
    start = time.time()
    response = client.post(f"/playbooks/{name}/run")
    if response.status_code != 200:
        print(f"run request failed: {response.status_code} {response.text[:200]}")
        return 1
    run = response.json()
    steps = run.get("steps", [])
    bad = [s for s in steps if s.get("passed") is False or s.get("error") or s.get("skipped")]
    heard = [s for s in steps if s.get("expect") == "message" and s.get("passed")]
    print(f"scene ok: {run.get('ok')} - {time.time() - start:.1f}s")
    print(f"{len(steps)} steps, {len(bad)} failed/skipped, {len(heard)} conversation assertions heard")
    for s in bad[:5]:
        detail = (s.get("error") or s.get("result") or "skipped")[:80]
        print(" ", s["index"], s["agent"], s["tool"], "->", detail)
    return 0 if run.get("ok") else 1


if __name__ == "__main__":
    sys.exit(main())
