import json

class Parser:
    @staticmethod
    def load_config(filepath):
        try:
            with open(filepath, 'r') as f:
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
