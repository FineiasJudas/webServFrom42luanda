#ifndef SERVER_HPP
#define SERVER_HPP

#include "../../includes/Headers.hpp"
#include "Poller.hpp"
#include "Connection.hpp"

class   Server
{
    private:
        int         server_fd;
        int         port;
        Poller      poller;
        std::string host;

        int     createSocket();
        void    setNonBlocking(int sockfd);
        void    handleNewConnection();
        void    handleClientData(Connection &conn);

    public:
        Server(const std::string &host, int port);
        ~Server();

        void    start();

};

#endif