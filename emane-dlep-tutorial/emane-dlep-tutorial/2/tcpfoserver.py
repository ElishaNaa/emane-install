import socket

def listen():
    connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connection.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    connection.setsockopt(socket.SOL_TCP, 23, 5)
    connection.bind(('0.0.0.0', 8000))
    connection.listen(10)
    while True:
        current_connection, address = connection.accept()
        while True:
            data = current_connection.recv(2048)
            if data:
                current_connection.send(data)
                print data
     	#current_connection.close()
     	#break


if __name__ == "__main__":
    try:
        listen()
    except KeyboardInterrupt:
        pass
