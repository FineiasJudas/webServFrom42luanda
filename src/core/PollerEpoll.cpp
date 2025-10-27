#include "../../includes/Headers.hpp"

Poller::Poller()
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
    {
        std::cerr << "Erro ao criar epoll: " << strerror(errno) << std::endl;
        return ;
    }
}

Poller::~Poller()
{
    if (epoll_fd != -1)
        close(epoll_fd);
}

void Poller::addFd(int fd, uint32_t events)
{
    struct epoll_event  ev;

    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        std::cerr << "Erro em epoll_ctl ADD: " << strerror(errno) << std::endl;
        return ;
    }
    std::cout << "FD " << fd << " adicionado ao epoll para monitoramento." << std::endl;
    std::cout << "Eventos monitorados: " << ((events & EPOLLIN) ? "EPOLLIN " : "")
        << ((events & EPOLLOUT) ? "EPOLLOUT " : "") << std::endl;
}

void Poller::removeFd(int fd)
{
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0) < 0)
        std::cerr << "Erro em epoll_ctl DEL: " << strerror(errno) << std::endl;
}

std::vector<struct epoll_event> Poller::wait(int timeout)
{
    int     nfds;
    struct epoll_event  events[MAX_EVENTS];

    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout);
    if (nfds < 0)
    {
        std::cerr << "Erro em epoll_wait: " << strerror(errno) << std::endl;
        return (std::vector<struct epoll_event>());
    }
    std::cout << "Eventos detectados: " << nfds << std::endl;
    return (std::vector<struct epoll_event>(events, events + nfds));
}