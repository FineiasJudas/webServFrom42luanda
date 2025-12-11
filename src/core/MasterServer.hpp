#ifndef MASTERSERVER_HPP
#define MASTERSERVER_HPP

#include <vector>
#include <map>
#include <string>
#include "Poller.hpp"
#include "Connection.hpp"
#include "../config/Config.hpp" // ServerConfig, LocationConfig
#include "../http/Request.hpp"

class   MasterServer
{
    public:
        MasterServer(const std::vector<ServerConfig> &servers);
        ~MasterServer();

        void    run();

    private:
        Poller  poller;
        std::map<int, ServerConfig *> listenFdToServers;
        std::map<int, Connection*> connections;

        int     read_timeout;
        int     write_timeout;
        int     keepalive_timeout;

    private:
        void    createListenSockets(const std::vector<ServerConfig> &servers);
        int     createListenSocketForPort(int port);

        void    handleAccept(int listenFd);
        void    handleRead(int clientFd);
        void    handleWrite(int clientFd);
        void    closeConnection(int clientFd);

        ServerConfig*   selectServerForRequest(const Request &req, int listenFd);

        void    checkTimeouts();
        bool    isListenFd(int fd) const;
        int     parsePortFromListenString(const std::string &s) const;
};

#endif


