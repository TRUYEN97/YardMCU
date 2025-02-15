import asyncio
import json
import logging
from aiohttp import web
from logging.handlers import TimedRotatingFileHandler
from datetime import datetime
import serial_asyncio
import requests

# Logger configuration
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
log_handler = TimedRotatingFileHandler(
    "server.log", when="midnight", interval=1, backupCount=30
)
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
log_handler.setFormatter(formatter)
logger.addHandler(log_handler)

# Configuration constants
SERIAL_PORT = 'COM12'
BAUD_RATE = 115200
HOST = '0.0.0.0'
PORT = 28686
AUTH_API_URL = "https://thuexetaplai.theworldbelongsto.us/api/v1/checkcard"

# Global variables
default_max_clients = 100
data_lock = asyncio.Lock()

class ClientManager:
    def __init__(self):
        self.clients = []  # List of (username, writer)
        self.special_client = None
        self.max_clients = default_max_clients

    async def add_client(self, username, writer):
        if len(self.clients) >= self.max_clients:
            return False

        self.clients.append((username, writer))
        if username == "special_client":
            self.special_client = writer
        return True

    async def remove_client(self, writer):
        for client in self.clients:
            if client[1] == writer:
                self.clients.remove(client)
                if writer == self.special_client:
                    self.special_client = None
                break

    async def broadcast(self, message):
        to_remove = []
        async with data_lock:
            for username, writer in self.clients:
                try:
                    print(message)
                    writer.write((message + "\r\n").encode('utf-8'))
                    await writer.drain()
                except Exception as e:
                    logger.error(f"Error broadcasting to {username}: {e}")
                    to_remove.append(writer)

            for writer in to_remove:
                await self.remove_client(writer)

    def list_clients(self):
        return [username for username, _ in self.clients]

    async def disconnect_client(self, username):
        for client in self.clients:
            if client[0] == username:
                writer = client[1]
                await self.remove_client(writer)
                writer.close()
                await writer.wait_closed()
                break

class Authenticator:
    def __init__(self, api_url):
        self.api_url = api_url

    def authenticate(self, username, password):
        try:
            # return True;
            response = requests.post(self.api_url, json={"id": username})
            if response.status_code == 200:
                return response.json().get("renting", False)
        except Exception as e:
            logger.error(f"Authentication API error: {e}")
        return False

class SerialHandler:
    def __init__(self, client_manager):
        self.client_manager = client_manager
        self.last_data = ""
        self.last_received_time = datetime.now().timestamp()

    async def handle_serial(self):
        while True:
            try:
                reader, writer = await serial_asyncio.open_serial_connection(url=SERIAL_PORT, baudrate=BAUD_RATE)
                logger.info(f"Connected to Serial {SERIAL_PORT} - {BAUD_RATE}")
                try:
                    while writer.transport.serial.is_open:
                        data = await reader.readline()
                        if data:
                            message = data.decode('utf-8').strip()
                            async with data_lock:
                                self.last_data = message
                            await self.client_manager.broadcast(message)
                except Exception as e:
                    logger.error(f"Error reading serial: {e}")
                writer.close()
                logger.info(f"Disconnected from Serial {SERIAL_PORT} - {BAUD_RATE}")
            except Exception as e:
                logger.error(f"Serial connection error: {e}")
                await asyncio.sleep(1)

class SocketServer:
    def __init__(self, host, port, client_manager, authenticator):
        self.host = host
        self.port = port
        self.client_manager = client_manager
        self.authenticator = authenticator

    async def handle_client(self, reader, writer):
        peername = writer.get_extra_info("peername")
        logger.info(f"Client {peername} connected...")

        try:
            auth_data = await reader.readline()
            auth_json = json.loads(auth_data.decode('utf-8').strip())
            username = auth_json.get("username")
            password = auth_json.get("password")

            if self.authenticator.authenticate(username, password):
                success = await self.client_manager.add_client(username, writer)
                if not success:
                    writer.write(json.dumps({"status": "error", "message": "Server full"}).encode('utf-8') + b"\r\n")
                    await writer.drain()
                    writer.close()
                    await writer.wait_closed()
                    return

                writer.write(json.dumps({"status": "success"}).encode('utf-8') + b"\r\n")
                await writer.drain()
                logger.info(f"Client {username} authenticated successfully!")
            else:
                writer.write(json.dumps({"status": "error", "message": "Unauthorized"}).encode('utf-8') + b"\r\n")
                await writer.drain()
                writer.close()
                await writer.wait_closed()
                logger.warning(f"Client {peername} authentication failed.")
                return

            while True:
                data = await reader.readline()
                if not data:
                    break

                message = data.decode('utf-8').strip()
                if writer == self.client_manager.special_client:
                    async with data_lock:
                        self.client_manager.last_data = message
                    await self.client_manager.broadcast(message)

        except Exception as e:
            logger.error(f"Error handling client {peername}: {e}")
        finally:
            logger.info(f"Client {peername} disconnected.")
            await self.client_manager.remove_client(writer)
            writer.close()
            await writer.wait_closed()

    async def start_server(self):
        server = await asyncio.start_server(self.handle_client, self.host, self.port)
        addr = server.sockets[0].getsockname()
        logger.info(f"Socket server running on {addr}")
        async with server:
            await server.serve_forever()

class AdminInterface:
    def __init__(self, client_manager):
        self.client_manager = client_manager

    async def handle_home(self, request):
        # Phục vụ trang HTML
        return web.FileResponse('./static/index.html')

    async def handle_get_clients(self, request):
        # Trả về danh sách client dưới dạng JSON
        return web.json_response(self.client_manager.list_clients())

    async def handle_disconnect_client(self, request):
        data = await request.json()
        username = data.get("username")
        if username:
            await self.client_manager.disconnect_client(username)
            return web.json_response({"status": "success"})
        return web.json_response({"status": "error", "message": "Invalid username"}, status=400)

    async def handle_update_max_clients(self, request):
        data = await request.json()
        max_clients = data.get("max_clients")
        if isinstance(max_clients, int) and max_clients > 0:
            self.client_manager.max_clients = max_clients
            return web.json_response({"status": "success"})
        return web.json_response({"status": "error", "message": "Invalid max_clients"}, status=400)

    def create_app(self):
        app = web.Application()
        app.router.add_static('/static', './static')  # Đường dẫn cho file tĩnh
        app.add_routes([
            web.get('/', self.handle_home),
            web.get('/clients', self.handle_get_clients),
            web.post('/disconnect', self.handle_disconnect_client),
            web.post('/update_max_clients', self.handle_update_max_clients),
        ])
        return app

async def main():
    client_manager = ClientManager()
    authenticator = Authenticator(AUTH_API_URL)
    serial_handler = SerialHandler(client_manager)
    socket_server = SocketServer(HOST, PORT, client_manager, authenticator)
    admin_interface = AdminInterface(client_manager)

    admin_app = admin_interface.create_app()
    admin_runner = web.AppRunner(admin_app)
    await admin_runner.setup()
    admin_site = web.TCPSite(admin_runner, HOST, 8080)
    await admin_site.start()

    await asyncio.gather(
        socket_server.start_server(),
        serial_handler.handle_serial()
    )

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("Server stopped.")
