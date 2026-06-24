import json
from pathlib import Path

class Parser:
    @staticmethod
    def load_config(filepath):
        requested_path = Path(filepath)
        ai_lab_root = Path(__file__).resolve().parent.parent
        candidate_paths = [requested_path]

        if not requested_path.is_absolute():
            candidate_paths.append(ai_lab_root / requested_path)

        resolved_path = None
        for candidate_path in candidate_paths:
            if candidate_path.exists():
                resolved_path = candidate_path
                break

        try:
            if resolved_path is None:
                raise FileNotFoundError(filepath)

            with resolved_path.open("r", encoding="utf-8") as f:
                return json.load(f)
        except FileNotFoundError:
            return {
                "port": 4242,
                "width": 10,
                "height": 10,
                "teams": ["Team1", "Team2", "Team3"],
                "initial_clients_per_team": 3,
                "default_freq": 100
            }
