#include "Connection.hpp"
#include <unistd.h>
#include <fcntl.h>

Connection::Connection(int fd)
    : fd(fd), close_after_send(false), has_error(false)
{
    fcntl(fd, F_SETFL, O_NONBLOCK);
    updateActivity();
}

Connection::~Connection()
{
    close(fd);
}

int Connection::getFd() const
{
    return (fd);
}

Buffer  &Connection::getInputBuffer()
{
    return (input);
}

Buffer  &Connection::getOutputBuffer()
{
    return (output);
}

ssize_t Connection::readFromFd()
{
    ssize_t     n;
    char    buf[4096];

    n = ::read(fd, buf, sizeof(buf));
    if (n > 0)
        input.append(buf, n);
    else if (n == 0)
        has_error = (true); // EOF
    return (n);
}

ssize_t Connection::writeToFd()
{
    ssize_t n;

    if (output.empty())
        return (0);
    n = ::write(fd, output.data(), output.size());
    if (n > 0)
        output.consume(n);
    else if (n == 0)
        has_error = (true);
    return (n);
}

void    Connection::setCloseAfterSend(bool value)
{
    close_after_send = value;
}

bool    Connection::shouldCloseAfterSend() const
{
    return (close_after_send);
}

bool    Connection::hasError() const
{
    return (has_error);
}

void    Connection::closeConnection()
{
    close(fd);
}

void    Connection::updateActivity()
{
    last_activity = std::time(NULL);
}
