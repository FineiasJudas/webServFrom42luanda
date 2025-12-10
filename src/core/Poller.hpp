#ifndef POLLER_HPP
#define POLLER_HPP

#include "../../includes/Headers.hpp"

struct  PollEvent
{
    int     fd;
    uint32_t    events;

    bool    isReadable() const { return events & EPOLLIN; }
    bool    isWritable() const { return events & EPOLLOUT; }
    bool    isError() const    { return events & (EPOLLHUP | EPOLLERR); }
};

class   Poller
{
    private:
        int     epfd;
        std::vector<struct epoll_event> eventBuffer;

    public:
        Poller();
        ~Poller();

        void    addFd(int fd, uint32_t events);
        void    modifyFd(int fd, uint32_t events);
        void    removeFd(int fd);

        std::vector<PollEvent>  waitEvents(int timeout_ms);

    private:
        void    makeNonBlocking(int fd);
};

#endif

