import socket
import threading
import serial
import time
import logging

# Cấu hình Logger
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("server.log"),  # Ghi log vào file "server.log"
        logging.StreamHandler()  # Hiển thị log ra màn hình console
    ]
)

logger = logging.getLogger(__name__)

# Cấu hình Serial (tùy chỉnh theo cổng serial và tốc độ baud bạn đang dùng)
SERIAL_PORT = '/dev/serial0'  # Cổng Serial trên Pi Zero W
BAUD_RATE = 115200

# Cấu hình Socket
HOST = '0.0.0.0'  # Cho phép kết nối từ tất cả các IP
PORT = 6868      # Cổng socket server

# Danh sách các kết nối client và lock
clients = []
clients_lock = threading.Lock()  # Khóa để đồng bộ danh sách clients

# Biến lưu trữ dữ liệu cũ và thời gian nhận dữ liệu cuối cùng
last_data = ""  # Dữ liệu nhận được cuối cùng từ serial
last_received_time = time.time()  # Thời gian nhận dữ liệu cuối cùng
data_lock = threading.Lock()  # Khóa cho dữ liệu để đảm bảo an toàn

# Hàm mở kết nối serial với cơ chế thử lại nếu thất bại
def open_serial():
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            logger.info("Đã kết nối lại với cổng serial.")
            return ser
        except serial.SerialException as e:
            logger.error(f"Lỗi kết nối với serial: {e}. Thử lại sau 5 giây...")
            time.sleep(5)

# Khởi tạo cổng serial
ser = open_serial()

def handle_client(client_socket, address):
    logger.info(f"Client {address} đã kết nối")
    with clients_lock:  # Sử dụng lock khi thêm client vào danh sách
        clients.append(client_socket)
    try:
        while True:
            # Chờ dữ liệu từ client (nếu cần xử lý dữ liệu từ client)
            data = client_socket.recv(1024)
            if not data:
                break
            print(f"Nhận từ {address}: {data.decode()}")
    except ConnectionResetError:
        logger.warning(f"Client {address} đã ngắt kết nối")
    finally:
        with clients_lock:  # Sử dụng lock khi xóa client khỏi danh sách
            clients.remove(client_socket)
        client_socket.close()

def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen(5)
    logger.info(f"Socket server đang chạy trên {HOST}:{PORT}")

    while True:
        client_socket, addr = server.accept()
        client_thread = threading.Thread(target=handle_client, args=(client_socket, addr))
        client_thread.start()

def read_serial():
    global ser, last_data, last_received_time
    while True:
        try:
            if ser.in_waiting > 0:
                # Đọc dữ liệu từ serial
                data = ser.readline().decode('utf-8', errors='ignore')
                print(f"Nhận từ Serial: {data}")
                # Cập nhật dữ liệu cũ và thời gian nhận cuối cùng
                with data_lock:
                    last_data = data
                    last_received_time = time.time()
                
                broadcast(data)
        except (serial.SerialException, OSError) as e:
            logger.error(f"Lỗi khi đọc từ Serial: {e}. Đang thử kết nối lại...")
            ser.close()
            ser = open_serial()  # Thử kết nối lại serial

def check_and_resend():
    """Kiểm tra nếu quá 1 giây không có dữ liệu mới từ serial thì gửi lại dữ liệu cũ."""
    global last_data, last_received_time
    while True:
        time.sleep(1)  # Kiểm tra mỗi giây một lần
        current_time = time.time()
        with data_lock:
            # Nếu đã hơn 1 giây kể từ khi nhận dữ liệu cuối cùng
            if current_time - last_received_time > 1 and last_data:
                logger.info("Không có dữ liệu mới, gửi lại dữ liệu cũ...")
                broadcast(last_data)

def broadcast(message):
    with clients_lock:  # Sử dụng lock để đảm bảo an toàn khi lặp qua danh sách
        for client in clients[:]:  # Lặp qua bản sao của danh sách
            try:
                client.sendall(message.encode('utf-8'))
            except BrokenPipeError:
                logger.warning("Lỗi khi gửi dữ liệu tới client.")
                with clients_lock:
                    clients.remove(client)

# Khởi tạo server và đọc serial trong các thread riêng biệt
server_thread = threading.Thread(target=start_server)
serial_thread = threading.Thread(target=read_serial)
resend_thread = threading.Thread(target=check_and_resend)

server_thread.start()
serial_thread.start()
resend_thread.start()

server_thread.join()
serial_thread.join()
resend_thread.join()
