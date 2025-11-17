#include "Connection.hpp"
#include <unistd.h>

Connection::Connection(int fd)
    : fd(fd), close_after_send(false)
{
    updateActivity();
}

Connection::~Connection()
{
    ::close(fd);
}

int     Connection::getFd() const
{
    return (fd);
}

Buffer  &Connection::getInputBuffer()
{
    return (input_buffer);
}

Buffer  &Connection::getOutputBuffer()
{
    return (output_buffer);
}

ssize_t Connection::readFromFd(char *buf, size_t size)
{
    ssize_t     bytes;

    bytes = ::read(fd, buf, size);
    if (bytes > 0)
    {
        updateActivity();
        input_buffer.append(buf, bytes);
    }
    return (bytes);
}

ssize_t Connection::writeToFd(const char *data, size_t size)
{
    ssize_t bytes = ::write(fd, data, size);
    if (bytes > 0)
        updateActivity();
    return (bytes);
}

void    Connection::setCloseAfterSend(bool v)
{
    close_after_send = v;
}

bool Connection::shouldCloseAfterSend() const
{
    return (close_after_send);
}

void    Connection::updateActivity()
{
    last_activity_time = time(NULL);
}
