import asyncio
from src.server import Server

if __name__ == "__main__":
    server = Server("config.json")
    try:
        asyncio.run(server.main_loop())
    except KeyboardInterrupt:
        pass
