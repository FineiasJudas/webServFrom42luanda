# tests/client.py
import socket
import sys

def send_request(host, port, method, path, headers=None, body=None):
    if headers is None:
        headers = {}

    # Monta cabeçalhos
    request_lines = [f"{method} {path} HTTP/1.1"]
    headers["Host"] = host
    headers["Connection"] = "close"
    if body:
        headers["Content-Length"] = str(len(body))

    for key, value in headers.items():
        request_lines.append(f"{key}: {value}")
    
    request_lines.append("\r\n")  # Fim dos cabeçalhos
    request = "\r\n".join(request_lines)
    if body:
        request += body

    # Conecta
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5)
    try:
        s.connect((host, port))
        s.sendall(request.encode())

        # Recebe resposta
        response = b""
        while True:
            data = s.recv(4096)
            if not data:
                break
            response += data

        print("=== RESPOSTA DO SERVIDOR ===")
        print(response.decode(errors='replace'))
    except Exception as e:
        print(f"Erro: {e}")
    finally:
        s.close()

# === TESTES PRONTOS ===
if __name__ == "__main__":
    HOST = "127.0.0.1"
    PORT = 8080

    print("Teste 1: GET /")
    send_request(HOST, PORT, "GET", "/")

    print("\nTeste 2: POST com corpo")
    send_request(
        HOST, PORT, "POST", "/upload",
        headers={"Content-Type": "text/plain"},
        body="Olá, WebServ!"
    )

    print("\nTeste 3: DELETE")
    send_request(HOST, PORT, "DELETE", "/file.txt")