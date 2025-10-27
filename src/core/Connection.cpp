#include "../../includes/Headers.hpp"

Connection::Connection(int fd) : fd(fd) {}

Connection::~Connection()
{
    close();
}

int Connection::getFd() const
{
    return (fd);
}

ssize_t Connection::read(char* buffer, size_t size)
{
    ssize_t     bytes;

    bytes = ::read(fd, buffer, size);
    if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        std::cerr << "Erro na leitura: " << strerror(errno) << std::endl;
    return (bytes);
}

ssize_t Connection::write(const char* buffer, size_t size)
{
    ssize_t     bytes;

    bytes = ::write(fd, buffer, size);
    if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        std::cerr << "Erro na escrita: " << strerror(errno) << std::endl;
    return (bytes);
}

void    Connection::close()
{
    if (fd != -1)
    {
        ::close(fd);
        fd = -1;
    }
}