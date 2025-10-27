#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

int main(void)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    int addrlen = sizeof(address);
    while (true)
    {
        listen(server_fd, 3);
        std::cout << "Aguardando conexões na porta 8080..." << std::endl;
        int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0)
        {
            std::cerr << "Erro ao aceitar conexão" << std::endl;
            continue ;
        }
        else
            std::cout << "Conexão aceita.." << std::endl;
        const char* msg = "Olá, cliente!\n";
        send(new_socket, msg, strlen(msg), 0);
        close(new_socket);
    }
    close(server_fd);
    return (0);
}
