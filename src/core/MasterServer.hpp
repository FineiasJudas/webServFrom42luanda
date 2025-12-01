#ifndef MASTERSERVER_HPP
#define MASTERSERVER_HPP

#include <vector>
#include <map>
#include "Poller.hpp"
#include "Connection.hpp"
#include "../config/Config.hpp"
#include "../http/Request.hpp"
#include "ListenSocket.hpp"

class   MasterServer
{
    private:
        Poller  poller;

        std::vector<int>    listenFds;    // lista de FDs de listen
        std::map<int, std::vector<ServerConfig*> >  listenFdToServer;
        std::map<int, Connection*>  connections;


        int     read_timeout;
        int     write_timeout;
        int     keepalive_timeout;

    public:
        MasterServer(const std::vector<ServerConfig> &servers);
        ~MasterServer();

        void    run();

    private:
        void    createListenSockets(const std::vector<ServerConfig>& servers);

        void    handleAccept(int listenFd);
        void    handleRead(int clientFd);
        void    handleWrite(int clientFd);
        void    closeConnection(int clientFd);

        ServerConfig    *selectServer(const Request& req, int listenFd);

        void    checkTimeouts();

        bool    isListenFd(int fd) const;
};

#endif

