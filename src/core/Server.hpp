#ifndef SERVER_HPP
#define SERVER_HPP

#include "../../includes/Headers.hpp"
#include "../config/Config.hpp"
#include "Poller.hpp"
#include "Connection.hpp"
#include "ListenSocket.hpp"
#include "../http/HttpParser.hpp"
#include "../http/Router.hpp"

class Server
{
private:
    std::vector<ListenSocket*> listenSockets;
    Poller poller;
    ServerConfig config;
    std::map<int, Connection*> connections;

public:
    explicit Server(const ServerConfig &conf);
    ~Server();

    void run();

private:
    void handleAccept(int listen_fd);
    void handleRead(int conn_fd);
    void handleWrite(int conn_fd);
};

#endif
