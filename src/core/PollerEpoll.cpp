#include "Poller.hpp"
#include <unistd.h>

Poller::Poller()
{
    epfd = epoll_create1(0);
    if (epfd < 0)
        throw std::runtime_error("epoll_create1 failed");

    // espaço para até 1024 eventos por ciclo
    eventBuffer.resize(1024);
}

Poller::~Poller()
{
    close(epfd);
}

void    Poller::makeNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0)
        flags = 0;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void    Poller::addFd(int fd, uint32_t events)
{
    makeNonBlocking(fd);

    struct epoll_event  ev;

    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
        throw std::runtime_error("epoll_ctl ADD failed");
}

void    Poller::modifyFd(int fd, uint32_t events)
{
    struct epoll_event  ev;

    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) < 0)
        throw std::runtime_error("epoll_ctl MOD failed");
}

void    Poller::removeFd(int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
}

std::vector<PollEvent>  Poller::waitEvents(int timeout_ms)
{
    std::vector<PollEvent>  result;

    int n = epoll_wait(epfd, &eventBuffer[0], eventBuffer.size(), timeout_ms);

    if (n < 0)
        return result;

    result.reserve(n);

    for (int i = 0; i < n; i++)
    {
        PollEvent   pe;
        pe.fd = eventBuffer[i].data.fd;
        pe.events = eventBuffer[i].events;
        result.push_back(pe);
    }

    return (result);
}
