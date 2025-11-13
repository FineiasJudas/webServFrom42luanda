#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

struct  Request
{
    std::string     method; // GET, POST, DELETE
    std::string     uri;
    std::string     http_version;
    std::map<std::string, std::string>  headers;
    std::string     body;
};

#endif
