#ifndef LISTENSOCKET_HPP
#define LISTENSOCKET_HPP

#include "ListenSocket.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

class ListenSocket
{
private:
    int fd;
    std::string address;
    int port;

public:
    ListenSocket(const std::string &addr, int port);
    ~ListenSocket();

    int getFd() const;
    std::string getAddress() const;
    int getPort() const;
};

#endif
