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
    bool    too_large_body;
    std::string     body;

    std::string path;   // apenas /delete-file
    std::map<std::string, std::string> query; // parâmetros da query-string
    Request()
        : header_end(0), too_large_body(false) {}
};

#endif

