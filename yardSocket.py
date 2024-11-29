import asyncio
import json
import logging
import serial_asyncio
from logging.handlers import TimedRotatingFileHandler
from datetime import datetime

# Cấu hình Logger
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

log_handler = TimedRotatingFileHandler(
    "server.log", when="midnight", interval=1, backupCount=30
)
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
log_handler.setFormatter(formatter)
logger.addHandler(log_handler)

# Cấu hình Serial
SERIAL_PORT = '/dev/serial0'
BAUD_RATE = 115200

# Cấu hình Socket
HOST = '0.0.0.0'
PORT = 6868
AUTHORIZED_CLIENTS = {"client": "f3cea34ed1507b50f09c236045bb1067", "special_client":"5756a05e4a1cc90f6dee0025a13a2f48"}
MAX_CLIENTS = 100  # Giới hạn số lượng client

# Biến toàn cục
clients = []  # Danh sách client (mỗi phần tử là tuple (username, writer))
special_client = None  # Client dặc biệt dể nhận dữ liệu

last_data = ""  # Dữ liệu nhận cuối cùng từ serial
last_received_time = datetime.now().timestamp()
data_lock = asyncio.Lock()

# Broadcast dữ liệu tới tất cả các client dã xác thực
async def broadcast(message):
    global clients, last_received_time
    to_remove = []
    async with data_lock:
        last_received_time = datetime.now().timestamp()
        for username, writer in clients:
            try:
                mess = message.encode('utf-8') + b"\r\n"
                writer.write(mess)
                await writer.drain()
            except Exception as e:
                logger.error(f"Loi khi gui toi client {username}: {e}")
                to_remove.append((username, writer))

        # Xóa các client không khả dụng
        for client in to_remove:
            clients.remove(client)

# Coroutine xử lý từng client
async def handle_client(reader, writer):
    global clients, special_client
    peername = writer.get_extra_info("peername")
    logger.info(f"Client {peername} dang ket noi...")

    # Kiểm tra nếu danh sách client dã dầy
    if len(clients) >= MAX_CLIENTS:
        logger.warning("So luong ket noi toi da. Tu troi ket noi")
        writer.write(json.dumps({"status": "error", "message": "Server full"}).encode('utf-8') + b"\r\n")
        await writer.drain()
        writer.close()
        await writer.wait_closed()
        return

    # Xác thực client
    try:
        auth_data = await reader.readline()
        auth_json = json.loads(auth_data.decode('utf-8').strip())
        username = auth_json.get("username")
        password = auth_json.get("password")

        if username in AUTHORIZED_CLIENTS and AUTHORIZED_CLIENTS[username] == password:
            writer.write(json.dumps({"status": "success"}).encode('utf-8') +  b"\r\n")
            await writer.drain()
            logger.info(f"Client {username} xac thuc thanh cong!")
            clients.append((username, writer))
            if username == "special_client":
                special_client = writer
        else:
            writer.write(json.dumps({"status": "error", "message": "Unauthorized"}).encode('utf-8') + b"\r\n")
            await writer.drain()
            writer.close()
            await writer.wait_closed()
            logger.warning(f"Client {peername} Xac thuc that bai.")
            return
    except Exception as e:
        logger.error(f"Loi khi xac thuc client {peername}: {e}")
        writer.close()
        await writer.wait_closed()
        return

    # Xử lý dữ liệu từ client
    try:
        while True:
            data = await reader.readline()
            if not data:
                break
            message = data.decode('utf-8').strip()

            # Nếu là client dặc biệt, xử lý dữ liệu như từ serial
            if writer == special_client:
                async with data_lock:
                    global last_data
                    last_data = message
                await broadcast(message)
    except Exception as e:
        logger.error(f"Loi client {username}: {e}")
    finally:
        logger.info(f"Client {username} Ngat ket noi.")
        clients.remove((username, writer))
        if writer == special_client:
            special_client = None
        writer.close()
        await writer.wait_closed()

# Coroutine chạy server
async def start_server():
    server = await asyncio.start_server(handle_client, HOST, PORT)
    addr = server.sockets[0].getsockname()
    logger.info(f"Server socket dang chay tren {addr}")
    async with server:
        await server.serve_forever()

# Coroutine dọc từ serial
async def handle_serial():
    global last_data
    while True:
        try:
            reader, writer = await serial_asyncio.open_serial_connection(url=SERIAL_PORT, baudrate=BAUD_RATE)
            logger.info(f"ket noi voi Serial {SERIAL_PORT} - {BAUD_RATE}")
            try:
                while writer.transport.serial.is_open:
                        data = await reader.readline()
                        if data:
                            message = data.decode('utf-8').strip()
                            async with data_lock:
                                last_data = message
                            await broadcast(message)
            except Exception as e:
                logger.error(f"Loi khi doc serial: {e}")
            writer.close()
            logger.info(f"ngat ket noi voi Serial {SERIAL_PORT} - {BAUD_RATE}")
        except Exception as e:
            await asyncio.sleep(1)

# Coroutine gửi lại dữ liệu cũ
async def resend_last_data():
    global last_data, last_received_time
    while True:
        await asyncio.sleep(1)
        current_time = datetime.now().timestamp()
        if current_time - last_received_time > 1 and last_data:
            await broadcast(last_data)

# Chương trình chính
async def main():
    
    await asyncio.gather(
        start_server(),
        handle_serial(),
        resend_last_data()
    )

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("Server stop.")
