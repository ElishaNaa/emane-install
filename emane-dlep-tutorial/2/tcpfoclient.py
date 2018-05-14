import socket

addr = ("10.100.0.1", 8000)
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.setsockopt(socket.SOL_TCP, 23, 5)

s.sendto("hello!",536870912,addr)
print s.recv(1000)

s.sendto("adding new mission",addr)
print s.recv(1000)

s.sendto("approving new mission",addr)
print s.recv(1000)

raw_input()
