#ifndef MASTERSERVER_HPP
#define MASTERSERVER_HPP

#include <map>
#include <vector>
#include <string>
#include <set>
#include "Poller.hpp"
#include "Connection.hpp"
#include "../http/Request.hpp"
#include "../config/Config.hpp"

struct  ListenAddress
{
    std::string ip;
    int     port;
};

class   MasterServer
{
    public:
        MasterServer(std::vector<ServerConfig> &servers);
        ~MasterServer();

        void    run();

    private:
        Poller  poller;
        std::map<int, ServerConfig *> listenFdToServers;
        std::map<int, Connection *> connections;
        std::map<int, int> cgiFdToClientFd;
        std::set<std::string> usedListenKeys;

        int     read_timeout;
        int     write_timeout;
        int     keepalive_timeout;

        void    createListenSockets(std::vector<ServerConfig> &servers);
        int createListenSocketForPort(const std::string &listen);

        void    handleAccept(int listenFd);
        void    handleRead(int clientFd);
        void    handleWrite(int clientFd);
        void    closeConnection(int clientFd);

        ServerConfig    *selectServerForRequest(const Request &req, int listenFd);

        void    checkTimeouts();
        void    checkCgiTimeouts();
        bool    isListenFd(int fd) const;
    
        void    handleCgiRead(int cgiFd);
        void    handleCgiWrite(int cgiFd);
        void    finalizeCgi(int clientFd);
};

#endif


