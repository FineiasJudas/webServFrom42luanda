#include "Connection.hpp"
#include <unistd.h>
#include "../http/Router.hpp"
#include "../http/HttpParser.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"

Connection::Connection(int fd)
    : fd(fd),
      listenFd(-1),
      server(NULL),
      closeAfterSend(false),
      last_activity_time(time(NULL)),
      waiting_for_write(false),
      write_start_time(0)
{
}

Connection::~Connection()
{
    ::close(fd);
}

ssize_t Connection::readFromFd()
{
    char    buffer[4096];

    ssize_t bytes = ::read(fd, buffer, sizeof(buffer));
    if (bytes > 0)
    {
        input_buffer.append(buffer, bytes);
        updateActivity();
    }
    return (bytes);
}

ssize_t Connection::writeToFd(const char* data, size_t size)
{
    ssize_t bytes = ::write(fd, data, size);
    if (bytes > 0)
        updateActivity();

    return (bytes);
}

int     Connection::getFd() const { return fd; }

void    Connection::setListenFd(int lf) { listenFd = lf; }

int     Connection::getListenFd() const { return listenFd; }

void    Connection::setServer(ServerConfig* s) { server = s; }

ServerConfig*   Connection::getServer() const { return server; }

Buffer  &Connection::getInputBuffer() { return input_buffer; }

Buffer  &Connection::getOutputBuffer() { return output_buffer; }

void    Connection::setCloseAfterSend(bool v) { closeAfterSend = v; }

bool    Connection::shouldCloseAfterSend() const { return closeAfterSend; }

void    Connection::updateActivity() { last_activity_time = time(NULL); }