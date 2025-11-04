#ifndef SERVER_HPP
#define SERVER_HPP

#include "../../includes/Headers.hpp"
#include "Poller.hpp"
#include "Connection.hpp"
#include <sys/epoll.h>
#include <vector>

class   Server
{
    private:
        int     server_fd;
        Poller  poller;
        std::map<int, Connection *>  connections;
        std::map<int, time_t>   last_activity;

        void    handleNewConnection();
        void    handleClientEvent(struct epoll_event &ev);
        void    tryParseAndRespond(Connection *conn);
        void    sendFromOutputBuffer(Connection *conn);

    public:
        Server(int port);
        ~Server();
        void    start();
};

#endif