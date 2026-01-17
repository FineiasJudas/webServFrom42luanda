#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <ctime>
#include "Buffer.hpp"
#include "../config/Config.hpp"

struct  CgiState
{
    pid_t   pid;
    int stdout_fd;
    int stdin_fd;
    std::string output;
    time_t  start_time;
    bool    stdin_closed;
    std::string pending_write;
    size_t  write_offset;
    
    CgiState() : pid(-1), stdout_fd(-1), stdin_fd(-1), 
                 start_time(0), stdin_closed(false), write_offset(0) {}
};

class   Connection
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
        bool    is_rejeting;
        CgiState    *cgi_state;

    public:
        Connection(int fd);
        ~Connection();

        int     getFd() const;
        void    setListenFd(int lf);
        int     getListenFd() const;

        void    setServer(ServerConfig *s);
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