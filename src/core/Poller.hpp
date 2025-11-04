#ifndef POLLER_HPP
#define POLLER_HPP

#include "../../includes/Headers.hpp"

#define MAX_EVENTS 10

class   Poller
{
    private:
        int epoll_fd;

    public:
        Poller();
        ~Poller();

        void    addFd(int fd, uint32_t events);
        void    modifyFd(int fd, uint32_t events);
        void    removeFd(int fd);
        std::vector<struct epoll_event> wait(int timeout);
};

#endif