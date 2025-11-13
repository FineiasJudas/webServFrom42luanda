#ifndef SERVER_HPP
#define SERVER_HPP

#include "../../includes/Headers.hpp"
#include "Connection.hpp"
#include "../http/HttpParser.hpp"
#include "../http/Router.hpp"
#include "../config/Config.hpp"
#include "Poller.hpp"

class   Server
{
    private:
        Poller  poller;
        std::map<int, ServerConfig> listeningSockets;
        std::map<int, Connection *> activeConnections;

    public:
        Server();
        ~Server();

        void    init(const Config &config);
        void    run();

    private:
        void    handleNewConnection(int listen_fd);
        void    handleRead(int client_fd);
        void    handleWrite(int client_fd);
        void    closeConnection(int client_fd);
};

#endif
