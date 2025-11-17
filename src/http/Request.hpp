#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "../../includes/Headers.hpp"

struct  Request
{
    std::string     method;
    std::string     uri;
    std::string     version;
    size_t     header_end;
    std::map<std::string, std::string>  headers;
    std::string     body;
};

#endif

