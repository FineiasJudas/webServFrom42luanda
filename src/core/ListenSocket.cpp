#include "ListenSocket.hpp"
#include <unistd.h>

ListenSocket::ListenSocket(const std::string &addr, int port)
    : fd(-1), address(addr), port(port)
{
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in  sa;

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (addr.empty() || addr == "0.0.0.0")
        sa.sin_addr.s_addr = INADDR_ANY;
    else
        sa.sin_addr.s_addr = inet_addr(addr.c_str());

    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        throw std::runtime_error("bind() failed");

    if (listen(fd, 128) < 0)
        throw std::runtime_error("listen() failed");

    // Setar o Socket como não bloqueante
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

ListenSocket::~ListenSocket()
{
    if (fd >= 0)
        close(fd);
}

int ListenSocket::getFd() const { return (fd); }

std::string ListenSocket::getAddress() const { return (address); }

int ListenSocket::getPort() const { return (port); }
