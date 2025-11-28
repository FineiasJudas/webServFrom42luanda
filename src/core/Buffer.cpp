#include "Buffer.hpp"

void    Buffer::append(const char *src, size_t len)
{
    if (len == 0)
        return ;
    contentData.insert(contentData.end(), src, src + len);
}

void    Buffer::append(const std::string &str)
{
    append(str.c_str(), str.size());
}

const std::vector<char> &Buffer::getData() const{ return (contentData); }

char    *Buffer::data() { return contentData.empty() ? NULL : &contentData[0]; }

size_t  Buffer::size() const { return contentData.size(); }

bool    Buffer::empty() const { return contentData.empty(); }

void    Buffer::clear() { contentData.clear(); }

void    Buffer::consume(size_t len)
{
    if (len >= contentData.size())
    {
        contentData.clear();
        return ;
    }
    contentData.erase(contentData.begin(), contentData.begin() + len);
}

std::string Buffer::toString() const
{
    if (contentData.empty())
        return std::string();
    return std::string(contentData.begin(), contentData.end());
}

size_t  Buffer::find(const std::string &delim) const
{
    std::string s = toString();

    return s.find(delim);
}

