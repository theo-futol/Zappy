import asyncio
from pathlib import Path

from src.server import Server

if __name__ == "__main__":
    config_path = Path(__file__).resolve().with_name("config.json")
    server = Server(str(config_path))
    try:
        asyncio.run(server.main_loop())
    except KeyboardInterrupt:
        pass
