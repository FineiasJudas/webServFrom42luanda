#ifndef HTTPPARSER_HPP
#define HTTPPARSER_HPP

#include "../../includes/Headers.hpp"
#include "../core/Buffer.hpp"
#include "Request.hpp"

class   HttpParser
{
    public:
        static bool hasCompleteRequest(const Buffer &buffer);
        static bool parseRequest(Buffer &buffer, Request &req, size_t max_body_size);

    private:
        static bool parseChunkedBody(Buffer &buffer, Request &req);
};


#endif
