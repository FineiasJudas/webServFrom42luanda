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

        // listen fd -> vector de ServerConfig* que ouvem nessa porta
        std::map<int, ServerConfig *> listenFdToServers;

        // client fd -> Connection*
        std::map<int, Connection*> connections;

        // tempoouts (segundos)
        int     read_timeout;       // tempo máximo sem completar um request
        int     write_timeout;      // tempo máximo bloqueado em escrita
        int     keepalive_timeout;  // tempo de keep-alive entre requests

    private:
        void    createListenSockets(const std::vector<ServerConfig> &servers);
        int     createListenSocketForPort(int port);

        void    handleAccept(int listenFd);
        void    handleRead(int clientFd);
        void    handleWrite(int clientFd);
        void    closeConnection(int clientFd);

        ServerConfig*   selectServerForRequest(const Request &req, int listenFd);

        void    checkTimeouts();

        // util
        bool    isListenFd(int fd) const;
        int     parsePortFromListenString(const std::string &s) const;
};

#endif


