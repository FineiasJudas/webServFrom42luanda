#include "../../includes/Headers.hpp"

Buffer::Buffer() {}

void    Buffer::append(const char* data, size_t size)
{
    this->data.insert(this->data.end(), data, data + size);
}

const std::vector<char> &Buffer::getData() const
{
    return (data);
} 

void    Buffer::clear()
{
    data.clear();
}