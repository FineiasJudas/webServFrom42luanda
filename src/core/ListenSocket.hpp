#ifndef LISTENSOCKET_HPP
#define LISTENSOCKET_HPP

#include "../../includes/Headers.hpp"

class   ListenSocket
{
    private:
        int     fd;
        std::string address;
        int     port;

    public:
        ListenSocket(const std::string &addr, int port);
        ~ListenSocket();

        int     getFd() const;
        std::string getAddress() const;
        int     getPort() const;

};

#endif
