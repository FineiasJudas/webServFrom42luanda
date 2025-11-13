#include "Poller.hpp"
#include <stdexcept>
#include <iostream>

Poller::Poller() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
        throw std::runtime_error("epoll_create1 failed");
}

Poller::~Poller() {
    close(epoll_fd);
}

void Poller::addFd(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
        throw std::runtime_error("epoll_ctl ADD failed");
}

void Poller::modifyFd(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1)
        throw std::runtime_error("epoll_ctl MOD failed");
}

void Poller::removeFd(int fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
        throw std::runtime_error("epoll_ctl DEL failed");
}

std::vector<struct epoll_event> Poller::wait(int timeout) {
    struct epoll_event events[MAX_EVENTS];
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout);
    if (nfds < 0)
        throw std::runtime_error("epoll_wait failed");

    std::vector<struct epoll_event> result;
    for (int i = 0; i < nfds; ++i)
        result.push_back(events[i]);

    return result;
}
