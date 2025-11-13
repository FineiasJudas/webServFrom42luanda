#include "Buffer.hpp"

void    Buffer::append(const char *src, size_t len)
{
    content.insert(content.end(), src, src + len);
}

void    Buffer::append(const std::string &str)
{
    content.insert(content.end(), str.begin(), str.end());
}

const std::vector<char> &Buffer::getData() const
{
    return (content);
}

char    *Buffer::data()
{
    return (&content[0]);
}

size_t  Buffer::size() const
{
    return (content.size());
}

bool    Buffer::empty() const
{
    return (content.empty());
}

void    Buffer::clear()
{
    content.clear();
}

void    Buffer::consume(size_t len)
{
    if (len >= content.size())
        content.clear();
    else
        content.erase(content.begin(), content.begin() + len);
}

std::string Buffer::toString() const
{
    return (std::string(content.begin(), content.end()));
}

bool    Buffer::contains(const std::string &delim) const
{
    return (find(delim) != std::string::npos);
}

size_t  Buffer::find(const std::string &delim) const
{
    std::string tmp(content.begin(), content.end());
    return (tmp.find(delim));
}

std::string Buffer::consumeUntil(const std::string &delim)
{
    size_t pos = find(delim);

    if (pos == std::string::npos)
        return ("");
    std::string res(content.begin(), content.begin() + pos);
    consume(pos + delim.size());
    return (res);
}
