#ifndef MASTER_SERVER_HPP
#define MASTER_SERVER_HPP

#include <vector>
#include <map>
#include <string>

#include "Poller.hpp"
#include "../../includes/Headers.hpp"
#include "ListenSocket.hpp"
#include "Connection.hpp"
#include "../config/Config.hpp"

class   MasterServer
{
    private:
        Poller  poller;

        // Todos os listen sockets (uma porta pode aparecer em mais de um server block)
        std::vector<ListenSocket*> listenSockets;

        // Mapeia FD → Connection*
        std::map<int, Connection *> connections;

        // Mapeia listen FD → qual server config usar
        std::map<int, ServerConfig *> listenFdToServer;

        int     read_timeout;      // <--- NOVO
        int     keepalive_timeout; // <--- NOVO

    public:
        MasterServer(const std::vector<ServerConfig> &servers);
        ~MasterServer();

        void run();

    private:
        void    createListenSockets(const std::vector<ServerConfig> &servers);
        void    handleAccept(int listenFd);
        void    handleRead(int clientFd);
        void    handleWrite(int clientFd);
        void    closeConnection(int clientFd);

        void checkWriteTimeouts();
};

#endif
