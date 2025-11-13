#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "../../includes/Headers.hpp"
#include "Buffer.hpp"

class Connection {
private:
    int fd;
    Buffer input_buffer;
    Buffer output_buffer;

public:
    explicit Connection(int fd);
    ~Connection();

    int getFd() const;
    Buffer &getInputBuffer();
    Buffer &getOutputBuffer();
    ssize_t read(char *buffer, size_t size);
    ssize_t write(const char *data, size_t size);
};

#endif
