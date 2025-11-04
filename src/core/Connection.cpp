#include "../../includes/Headers.hpp"
// src/core/Connection.cpp
#include "Connection.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

Connection::Connection(int fd) : fd(fd), close_after_send(false)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        std::cerr << "Erro ao configurar O_NONBLOCK\n";
    }
}

Connection::~Connection()
{
    close();
}

int Connection::getFd() const
{
    return (fd);
}

Buffer &Connection::getInputBuffer()
{
    return (input_buffer);
}

Buffer  &Connection::getOutputBuffer()
{
    return (output_buffer);
}

void    Connection::setCloseAfterSend(bool value)
{
    close_after_send = value;
}

bool    Connection::shouldCloseAfterSend() const
{
    return (close_after_send);
}

ssize_t Connection::read(char* buffer, size_t size)
{
    return (::read(fd, buffer, size));  // NÃO CHECA errno!
}

ssize_t Connection::write(const char* data, size_t size)
{
    return (::write(fd, data, size));   // NÃO CHECA errno!
}

void    Connection::close()
{
    if (fd != -1)
    {
        ::close(fd);
        fd = -1;
    }
}