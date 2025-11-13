#include "Poller.hpp"
#include <unistd.h>

Poller::Poller() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
        throw std::runtime_error("Erro ao criar epoll");
}

Poller::~Poller() { close(epoll_fd); }

void Poller::addFd(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

void Poller::modifyFd(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void Poller::removeFd(int fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

std::vector<struct epoll_event> Poller::wait(int timeout) {
    struct epoll_event events[MAX_EVENTS];
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout);
    std::vector<struct epoll_event> result;
    for (int i = 0; i < nfds; ++i)
        result.push_back(events[i]);
    return result;
}
