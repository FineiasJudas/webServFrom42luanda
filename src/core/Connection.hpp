#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <string>
# include <ctime>
# include "Buffer.hpp"

class   Connection
{
    private:
        int     fd;
        Buffer  input;
        Buffer  output;
        bool    close_after_send;
        bool    has_error;
        time_t  last_activity;

    public:
        explicit    Connection(int fd);
        ~Connection();

        int     getFd() const;
        Buffer      &getInputBuffer();
        Buffer      &getOutputBuffer();

        ssize_t     readFromFd();
        ssize_t     writeToFd();

        void    setCloseAfterSend(bool value);
        bool    shouldCloseAfterSend() const;
        bool    hasError() const;
        void    closeConnection();
        void    updateActivity();
};

#endif
