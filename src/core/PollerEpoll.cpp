#include "../../includes/Headers.hpp"

Poller::Poller() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        std::cerr << "Erro em epoll_create1\n";
    }
}

Poller::~Poller() {
    if (epoll_fd != -1) {
        close(epoll_fd);
    }
}

void Poller::addFd(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "Erro em epoll_ctl ADD\n";
    } else {
        std::cout << "FD " << fd << " adicionado ao epoll (eventos: " 
                  << (events & EPOLLIN ? "IN " : "")
                  << (events & EPOLLOUT ? "OUT " : "") << ")\n";
    }
}

void Poller::modifyFd(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        std::cerr << "Erro em epoll_ctl MOD\n";
    }
}

void Poller::removeFd(int fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
}

std::vector<struct epoll_event> Poller::wait(int timeout) {
    struct epoll_event events[MAX_EVENTS];
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout);
    if (nfds < 0) {
        std::cerr << "Erro em epoll_wait\n";
        return std::vector<struct epoll_event>();
    }
    std::cout << "Eventos detectados: " << nfds << "\n";
    return std::vector<struct epoll_event>(events, events + nfds);
}