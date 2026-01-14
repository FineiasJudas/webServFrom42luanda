#ifndef MASTERSERVER_HPP
#define MASTERSERVER_HPP

#include <map>
#include <vector>
#include <string>
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
        MasterServer(const std::vector<ServerConfig> &servers);
        ~MasterServer();

        void    run();

    private:
        Poller  poller;
        std::map<int, ServerConfig *> listenFdToServers;
        std::map<int, Connection *> connections;
        std::map<int, int> cgiFdToClientFd;
        std::vector<int>    ports;

        int     read_timeout;
        int     write_timeout;
        int     keepalive_timeout;

        void    createListenSockets(const std::vector<ServerConfig> &servers);
        int     createListenSocket(const std::string &ip, int port);

        void    handleAccept(int listenFd);
        void    handleRead(int clientFd);
        void    handleWrite(int clientFd);
        void    closeConnection(int clientFd);

        ServerConfig    *selectServerForRequest(const Request &req, int listenFd);

        void    checkTimeouts();
        void    checkCgiTimeouts();
        bool    isListenFd(int fd) const;
        int     parsePortFromListenString(const std::string &s) const;

        ListenAddress  parseListen(const std::string &s);
    
        void    handleCgiRead(int cgiFd);
        void    handleCgiWrite(int cgiFd);
        void    finalizeCgi(int clientFd);
};

#endif


