#include "ListenSocket.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

ListenSocket::ListenSocket(const std::string &addr, int port)
    : fd(-1), address(addr), port(port)
{
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        throw std::runtime_error("Erro ao criar socket");

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = (addr.empty() || addr == "0.0.0.0")
                             ? INADDR_ANY
                             : inet_addr(addr.c_str());

    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        throw std::runtime_error("Erro ao dar bind no socket");

    if (listen(fd, 10) < 0)
        throw std::runtime_error("Erro ao fazer listen");

    std::cout << "🟢 Listening on " << (addr.empty() ? "0.0.0.0" : addr)
              << ":" << port << std::endl;
}

ListenSocket::~ListenSocket()
{
    if (fd >= 0)
        close(fd);
}

int ListenSocket::getFd() const { return fd; }
std::string ListenSocket::getAddress() const { return address; }
int ListenSocket::getPort() const { return port; }
