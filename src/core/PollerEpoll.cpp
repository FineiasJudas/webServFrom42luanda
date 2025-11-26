#include "Poller.hpp"
#include <unistd.h>

Poller::Poller()
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
        throw std::runtime_error("epoll_create1 failed");
}

Poller::~Poller()
{
    if (epoll_fd >= 0)
        close(epoll_fd);
}

void    Poller::addFd(int fd, uint32_t events)
{
    struct epoll_event  ev;

    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
        throw std::runtime_error("epoll_ctl ADD failed");
}

void    Poller::modifyFd(int fd, uint32_t events)
{
    struct epoll_event ev;

    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1)
        throw std::runtime_error("epoll_ctl MOD failed");
}

void    Poller::removeFd(int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

std::vector<struct epoll_event> Poller::wait(int timeout)
{
    int     nfds;
    struct epoll_event  events[MAX_EVENTS];

    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout);
    std::vector<struct epoll_event>     result;
    if (nfds < 0)
        return (result);
    for (int i = 0; i < nfds; ++i)
        result.push_back(events[i]);
    return (result);
}
