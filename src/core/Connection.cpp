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
      write_start_time(0),
      is_rejeting(false),
      cgi_state(NULL)
{
}

Connection::~Connection()
{
    ::close(fd);
    if (cgi_state)
    {
        if (cgi_state->stdout_fd != -1)
            close(cgi_state->stdout_fd);
        if (cgi_state->stdin_fd != -1)
            close(cgi_state->stdin_fd);
        if (cgi_state->pid > 0)
        {
            kill(cgi_state->pid, SIGKILL);
            waitpid(cgi_state->pid, NULL, 0);
        }
        delete cgi_state;
    }
}

ssize_t Connection::readFromFd()
{
    ssize_t bytes;
    char    buffer[4096];

    bytes = ::read(fd, buffer, sizeof(buffer));
    if (bytes > 0)
    {
        input_buffer.append(buffer, bytes);
        updateActivity();
        return (bytes);
    } // leitura bem sucedida

    if (bytes == 0)
        return (0); // conexão fechada pelo cliente
    return (-1); // erro na leitura
}

ssize_t Connection::writeToFd(const char* data, size_t size)
{
    ssize_t     bytes;

    bytes = ::write(fd, data, size);
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