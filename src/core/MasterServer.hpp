#ifndef MASTERSERVER_HPP
#define MASTERSERVER_HPP

#include <map>
#include <vector>
#include <string>
#include "Poller.hpp"
#include "Connection.hpp"
#include "../http/Request.hpp"
#include "../config/Config.hpp"

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
        std::map<int, int> cgiFdToClientFd;  // fd do CGI -> fd do cliente
        std::vector<int>    ports;

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

        ServerConfig    *selectServerForRequest(const Request &req, int listenFd);

        void    checkTimeouts();
        void    checkCgiTimeouts();
        bool    isListenFd(int fd) const;
        int     parsePortFromListenString(const std::string &s) const;
    
    // No MasterServer.hpp
    private:
        
        void handleCgiRead(int cgiFd);
        void handleCgiWrite(int cgiFd);
        void finalizeCgi(int clientFd);
};

#endif


