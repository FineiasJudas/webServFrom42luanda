#ifndef SERVER_HPP
#define SERVER_HPP

#include "../../includes/Headers.hpp"
#include "ListenSocket.hpp"
#include "Connection.hpp"
#include "Poller.hpp"
#include "../config/Config.hpp"
#include <map>

class   Server
{
    private:
        ServerConfig                config;
        Poller                      poller;

        std::vector<ListenSocket *>  listenSockets;
        std::map<int, Connection *>  connections;

        int     read_timeout;      // <--- NOVO
        int     keepalive_timeout; // <--- NOVO

        static const int            WRITE_TIMEOUT_SECONDS = 30;

        void    checkWriteTimeouts();
        void    handleAccept(int listen_fd);
        void    handleRead(int fd);
        void    handleWrite(int fd);
        void    closeConnection(int conn_fd);

    public:
        Server(const ServerConfig &conf);
        ~Server();

        void    run();
        
};

#endif
