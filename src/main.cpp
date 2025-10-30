#include "../includes/Headers.hpp"


int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // 1. Criar socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Erro ao criar socket\n";
        return 1;
    }

    // 2. Reutilizar o endereço/porta
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    // 3. Configurar endereço
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // aceita conexões de qualquer IP
    address.sin_port = htons(8080);       // porta 8080

    // 4. Associar socket ao endereço
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Erro no bind\n";
        return 1;
    }

    // 5. Colocar o socket em modo de escuta
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Erro no listen\n";
        return 1;
    }

    std::cout << "Servidor ouvindo na porta 8080...\n";

    // 6. Aceitar conexões
    while (true) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            std::cerr << "Erro no accept\n";
            continue;
        }

        const char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, world!\n";
        send(new_socket, msg, strlen(msg), 0);
        close(new_socket);
    }

    close(server_fd);
    return 0;
}
