#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "../../includes/Headers.hpp"
#include "Buffer.hpp"

class   Connection
{
    private:
        int     fd;
        Buffer  input_buffer;
        Buffer  output_buffer;
        bool    close_after_send;

    public:
        explicit    Connection(int fd);
        ~Connection();

        int     getFd() const;
        Buffer  &getInputBuffer();
        Buffer  &getOutputBuffer();

        ssize_t     readFromFd(char *buf, size_t size);
        ssize_t     writeToFd(const char *data, size_t size);

        void    setCloseAfterSend(bool v);
        bool    shouldCloseAfterSend() const;
        void    updateActivity();

        long    last_activity_time;
};

#endif