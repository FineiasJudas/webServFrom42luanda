#include "Server.hpp"
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

/*static int parsePort(const std::string &listenStr)
{
    size_t colon = listenStr.find(':');
    if (colon != std::string::npos)
        return atoi(listenStr.substr(colon + 1).c_str());
    return atoi(listenStr.c_str());
}*/

// ------------------------------------------------------------
// Construtor: cria os sockets de escuta com base no config
// ------------------------------------------------------------
Server::Server(const ServerConfig &conf) : config(conf)
{
    for (size_t i = 0; i < conf.listen.size(); ++i)
    {
        std::string host = "0.0.0.0";
        std::string entry = conf.listen[i];
        int port = 8080;

        size_t colon = entry.find(':');
        if (colon != std::string::npos)
        {
            host = entry.substr(0, colon);
            port = atoi(entry.substr(colon + 1).c_str());
        }
        else
            port = atoi(entry.c_str());

        ListenSocket *sock = new ListenSocket(host, port);
        listenSockets.push_back(sock);
        poller.addFd(sock->getFd(), EPOLLIN);
    }

    std::cout << "Server inicializado com " << listenSockets.size()
              << " socket(s) de escuta.\n";
}

Server::~Server()
{
    for (size_t i = 0; i < listenSockets.size(); ++i)
        delete listenSockets[i];
    for (std::map<int, Connection*>::iterator it = connections.begin(); it != connections.end(); ++it)
        delete it->second;
}

// ------------------------------------------------------------
// Loop principal: aceita e trata conexões
// ------------------------------------------------------------
void Server::run()
{
    while (true)
    {
        std::vector<struct epoll_event> events = poller.wait(1000);
        for (size_t i = 0; i < events.size(); ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            // Verifica se é socket de escuta
            bool isListening = false;
            for (size_t j = 0; j < listenSockets.size(); ++j)
                if (fd == listenSockets[j]->getFd())
                    isListening = true;

            if (isListening)
                handleAccept(fd);
            else if (ev & EPOLLIN)
                handleRead(fd);
            else if (ev & EPOLLOUT)
                handleWrite(fd);
        }
    }
}

// ------------------------------------------------------------
// Accept
// ------------------------------------------------------------
void Server::handleAccept(int listen_fd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int client_fd = accept(listen_fd, (struct sockaddr *)&addr, &len);
    if (client_fd < 0)
        return;

    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    poller.addFd(client_fd, EPOLLIN);
    connections[client_fd] = new Connection(client_fd);
    std::cout << "🧩 Nova conexão: FD " << client_fd << std::endl;
}

// ------------------------------------------------------------
// Read (recebe e processa request HTTP)
// ------------------------------------------------------------
void Server::handleRead(int conn_fd)
{
    char buffer[4096];
    ssize_t bytes = connections[conn_fd]->read(buffer, sizeof(buffer));
    if (bytes <= 0)
    {
        poller.removeFd(conn_fd);
        close(conn_fd);
        delete connections[conn_fd];
        connections.erase(conn_fd);
        return;
    }

    Request req;
    Buffer &input = connections[conn_fd]->getInputBuffer();
    input.append(buffer, bytes);

    if (HttpParser::hasCompleteRequest(input))
    {
        if (HttpParser::parseRequest(input, req, config.max_body_size))
        {
            Response res = Router::route(req, config);
            std::string resStr = res.toString();
            connections[conn_fd]->getOutputBuffer().append(resStr);
            poller.modifyFd(conn_fd, EPOLLOUT);
        }
    }
}

// ------------------------------------------------------------
// Write (envia resposta HTTP)
// ------------------------------------------------------------
void Server::handleWrite(int conn_fd)
{
    Buffer &out = connections[conn_fd]->getOutputBuffer();
    ssize_t sent = connections[conn_fd]->write(out.data(), out.size());
    if (sent > 0)
        out.consume(sent);

    if (out.empty())
    {
        poller.removeFd(conn_fd);
        close(conn_fd);
        delete connections[conn_fd];
        connections.erase(conn_fd);
    }
}
