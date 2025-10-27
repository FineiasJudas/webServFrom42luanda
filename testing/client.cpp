#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main() {
    // Cria socket TCP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Erro ao criar socket: " << strerror(errno) << std::endl;
        return 1;
    }

    // Configura endereço do servidor
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080); // Porta do servidor
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Endereço localhost

    // Conecta ao servidor
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Erro ao conectar: " << strerror(errno) << std::endl;
        close(sockfd);
        return 1;
    }
    std::cout << "Conectado ao servidor!" << std::endl;

    // Envia uma mensagem
    const char* message = "Hello, server!\n";
    ssize_t bytes_sent = send(sockfd, message, strlen(message), 0);
    if (bytes_sent < 0) {
        std::cerr << "Erro ao enviar: " << strerror(errno) << std::endl;
        close(sockfd);
        return 1;
    }
    std::cout << "Enviado: " << message;

    // Recebe a resposta
    char buffer[1024];
    ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        std::cerr << "Erro ao receber: " << strerror(errno) << std::endl;
        close(sockfd);
        return 1;
    }
    if (bytes_received == 0) {
        std::cout << "Conexão fechada pelo servidor" << std::endl;
    } else {
        buffer[bytes_received] = '\0'; // Adiciona terminador nulo
        std::cout << "Resposta recebida: " << buffer << std::endl;
    }

    // Fecha o socket
    close(sockfd);
    return 0;
}