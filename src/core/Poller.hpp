#ifndef POLLER_HPP
#define POLLER_HPP

#include "../../includes/Headers.hpp"

class   Poller
{
    private:
        int     epoll_fd; // File descriptor do epoll
        static const int    MAX_EVENTS = 10; // Máximo de eventos por wait

    public:
        Poller();
        ~Poller();
        void    addFd(int fd, uint32_t events); // Adiciona FD para monitoramento
        void    removeFd(int fd); // Remove FD
        std::vector<struct epoll_event> wait(int timeout); // Espera por eventos

};

#endif