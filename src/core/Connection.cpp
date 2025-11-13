#include "Connection.hpp"
#include <unistd.h>

Connection::Connection(int fd) : fd(fd) {}
Connection::~Connection() { close(fd); }

int Connection::getFd() const { return fd; }
Buffer &Connection::getInputBuffer() { return input_buffer; }
Buffer &Connection::getOutputBuffer() { return output_buffer; }

ssize_t Connection::read(char *buffer, size_t size) {
    ssize_t bytes = ::read(fd, buffer, size);
    if (bytes > 0)
        input_buffer.append(buffer, bytes);
    return bytes;
}

ssize_t Connection::write(const char *data, size_t size) {
    return ::write(fd, data, size);
}
