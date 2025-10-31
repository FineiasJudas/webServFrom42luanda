#ifndef POLLER_HPP
#define POLLER_HPP

#include "../../includes/Headers.hpp"
#include <sys/epoll.h>
#include <vector>

#define MAX_EVENTS 10

class   Poller
{
    private:
        int epoll_fd;

    public:
        Poller();
        ~Poller();

        void    addFd(int fd, uint32_t events);
        void    modifyFd(int fd, uint32_t events);  // <--- NOVO
        void    removeFd(int fd);
        std::vector<struct epoll_event> wait(int timeout);
};

#endif