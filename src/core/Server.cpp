#include "../http/HttpParser.hpp"
#include "../http/Response.hpp"
#include "../http/Router.hpp"
#include "Server.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
Server::Server() {}

Server::~Server() {
    for (std::map<int, Connection*>::iterator it = activeConnections.begin();
         it != activeConnections.end(); ++it) {
        delete it->second;
    }
}

// -------------------------------------------------------------
// 1️⃣ Inicializa os sockets (para cada server block do config)
// -------------------------------------------------------------
void Server::init(const Config &config) {
    for (size_t i = 0; i < config.servers.size(); ++i) {
        const ServerConfig &srv = config.servers[i];
        for (size_t j = 0; j < srv.listen.size(); ++j) {
            std::string addrPort = srv.listen[j];
            int port = 8080;

            // parsing de "ip:port" ou ":port"
            size_t colon = addrPort.find(':');
            if (colon != std::string::npos)
                port = std::atoi(addrPort.substr(colon + 1).c_str());
            else
                port = std::atoi(addrPort.c_str());

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
                throw std::runtime_error("Socket creation failed");

            int opt = 1;
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;

            if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
                throw std::runtime_error("Bind failed");
            if (listen(sock, 10) < 0)
                throw std::runtime_error("Listen failed");

            // modo não-bloqueante
            fcntl(sock, F_SETFL, O_NONBLOCK);

            poller.addFd(sock, EPOLLIN);
            listeningSockets[sock] = srv;
            std::cout << "Listening on port " << port << std::endl;
        }
    }
}

// -------------------------------------------------------------
// 2️⃣ Loop principal (epoll)
// -------------------------------------------------------------
void Server::run() {
    std::cout << "Server running..." << std::endl;

    while (true) {
        std::vector<struct epoll_event> events = poller.wait(1000);
        for (size_t i = 0; i < events.size(); ++i) {
            int fd = events[i].data.fd;

            if (listeningSockets.count(fd))
                handleNewConnection(fd);
            else if (events[i].events & EPOLLIN)
                handleRead(fd);
            else if (events[i].events & EPOLLOUT)
                handleWrite(fd);
        }
    }
}

// -------------------------------------------------------------
// 3️⃣ Aceita novas conexões
// -------------------------------------------------------------
void Server::handleNewConnection(int listen_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0)
        return;

    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    poller.addFd(client_fd, EPOLLIN);
    activeConnections[client_fd] = new Connection(client_fd);
    std::cout << "New connection on fd " << client_fd << std::endl;
}

// -------------------------------------------------------------
// 4️⃣ Lê dados de um cliente e processa o request
// -------------------------------------------------------------
void Server::handleRead(int client_fd)
{
    Connection *conn = activeConnections[client_fd];
    char tmp[4096];
    ssize_t bytes = recv(client_fd, tmp, sizeof(tmp), 0);

    if (bytes <= 0) {
        closeConnection(client_fd);
        return;
    }

    conn->getInputBuffer().append(tmp, bytes);

    // Enquanto houver requisições completas no buffer
    while (HttpParser::hasCompleteRequest(conn->getInputBuffer())) {
        Request req;
        ServerConfig srv = listeningSockets.begin()->second;
        if (!HttpParser::parseRequest(conn->getInputBuffer(), req, srv.max_body_size))
            break;

        Response res = Router::route(req, srv);
        std::string response_str = res.toString();
        conn->getOutputBuffer().append(response_str.c_str(), response_str.size());
        poller.modifyFd(client_fd, EPOLLOUT);
    }
}

// -------------------------------------------------------------
// 5️⃣ Envia resposta HTTP ao cliente
// -------------------------------------------------------------
void    Server::handleWrite(int client_fd)
{
    ssize_t sent;

    Connection *conn = activeConnections[client_fd];
    Buffer &out = conn->getOutputBuffer();

    if (out.empty())
    {
        closeConnection(client_fd);
        return ;
    }

    sent = send(client_fd, out.data(), out.size(), 0);
    if (sent > 0)
        out.consume(sent);

    if (out.empty())
        closeConnection(client_fd);
}

// -------------------------------------------------------------
// 6️⃣ Fecha conexão e limpa
// -------------------------------------------------------------
void    Server::closeConnection(int client_fd)
{
    poller.removeFd(client_fd);
    close(client_fd);
    delete activeConnections[client_fd];
    activeConnections.erase(client_fd);
    std::cout << "Conexão de fd=" << client_fd << " fechada" << std::endl;
}
