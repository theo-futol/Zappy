import asyncio

class ClientHandler:
    def __init__(self, reader, writer, server, client_id):
        self.reader = reader
        self.writer = writer
        self.server = server
        self.client_id = client_id
        self.player = None
        self.team = None

    def send_response(self, msg):
        try:
            self.writer.write(msg.encode())
        except:
            pass

    async def run(self):
        self.send_response("WELCOME\n")
        try:
            data = await self.reader.readline()
            if not data:
                return
            team_name = data.decode().strip()
            
            if team_name == "GRAPHIC":
                self.team = "GRAPHIC"
                return
                
            if team_name not in self.server.teams:
                self.send_response("ko\n")
                self.writer.close()
                return
                
            slots = self.server.get_available_slots(team_name)
            if slots <= 0:
                self.send_response("ko\n")
                self.writer.close()
                return
                
            self.team = team_name
            self.server.consume_slot(team_name)
            
            self.send_response(f"{slots - 1}\n")
            self.send_response(f"{self.server.width} {self.server.height}\n")
            
            self.player = self.server.spawn_player(self.client_id, self.team)
            
            while True:
                data = await self.reader.readline()
                if not data:
                    break
                cmd = data.decode().strip()
                self.player.enqueue_command(cmd)

        except ConnectionResetError:
            pass
        finally:
            if self.player:
                self.player.is_dead = True
            self.writer.close()
