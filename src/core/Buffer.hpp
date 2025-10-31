#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "../../includes/Headers.hpp"
#include <vector>
#include <string>

class Buffer {
private:
    std::vector<char> contentData;

public:
    void append(const char* src, size_t len);
    void append(const std::string& str);
    const std::vector<char>& getData() const;
    char* data();
    size_t size() const;
    bool empty() const;
    void clear();
    void consume(size_t len);  // Remove bytes do início
    std::string toString() const;
    bool contains(const std::string& delim) const;
};

#endif