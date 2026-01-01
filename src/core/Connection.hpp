#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <ctime>
#include "Buffer.hpp"
#include "../config/Config.hpp"

class Connection
{
    private:
        int     fd;
        int     listenFd;
        ServerConfig    *server;
        bool    closeAfterSend;

        Buffer  input_buffer;
        Buffer  output_buffer;

    public:
        time_t  last_activity_time;
        bool    waiting_for_write;
        time_t  write_start_time;

    public:
        Connection(int fd);
        ~Connection();

        int     getFd() const;
        void    setListenFd(int lf);
        int     getListenFd() const;

        void    setServer(ServerConfig* s);
        ServerConfig    *getServer() const;

        Buffer  &getInputBuffer();
        Buffer  &getOutputBuffer();

        void    setCloseAfterSend(bool v);
        bool    shouldCloseAfterSend() const;
        

        ssize_t     readFromFd();
        ssize_t     writeToFd(const char *data, size_t size);

        void    updateActivity();

};

#endif