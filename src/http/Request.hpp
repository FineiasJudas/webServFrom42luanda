#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "../../includes/Headers.hpp"

struct Request
{
    std::string method;
    std::string uri;
    std::string path;
    std::string version;
    std::string query_string;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query;
    std::string body;
    
    size_t  header_end;
    bool too_large_body;
    
    bool bad_request;
    std::string bad_request_reason;
    
    Request() : header_end(0), too_large_body(false), bad_request(false) {}
};

#endif

