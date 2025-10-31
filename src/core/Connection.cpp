#include "../../includes/Headers.hpp"

Connection::Connection(int fd) : fd(fd) {
    // Garante não-bloqueante
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

Connection::~Connection() {
    close();
}

int Connection::getFd() const {
    return fd;
}

Buffer& Connection::getInputBuffer() {
    return input_buffer;
}

Buffer& Connection::getOutputBuffer() {
    return output_buffer;
}

ssize_t Connection::read(char* buffer, size_t size) {
    ssize_t bytes = ::read(fd, buffer, size);
    // NÃO CHECA errno! (proibido)
    return bytes;
}

ssize_t Connection::write(const char* data, size_t size) {
    ssize_t bytes = ::write(fd, data, size);
    // NÃO CHECA errno!
    return bytes;
}

void Connection::close() {
    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
}