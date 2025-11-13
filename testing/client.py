import socket, os

def test_get():
    req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
    s = socket.socket()
    s.connect(('127.0.0.1', 8080))
    s.send(req.encode())
    print(s.recv(1024).decode())
    s.close()

def test_upload():
    boundary = "----boundary"
    body = f"--{boundary}\r\nContent-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n\r\nHello Upload!\r\n--{boundary}--\r\n".encode()
    req = f"POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Type: multipart/form-data; boundary={boundary}\r\nContent-Length: {len(body)}\r\n\r\n".encode()
    s = socket.socket()
    s.connect(('127.0.0.1', 8080))
    s.send(req + body)
    print(s.recv(1024).decode())
    s.close()

test_get()
test_upload()