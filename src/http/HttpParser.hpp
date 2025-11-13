#ifndef HTTPPARSER_HPP
# define HTTPPARSER_HPP

#include <string>
#include <map>
#include "../core/Buffer.hpp"
#include "Request.hpp"

class   HttpParser
{
    public:
        // verifica se o buffer tem um request completo
        static bool hasCompleteRequest(const Buffer &buffer);

        // consome um request completo do buffer e preenche o objeto Request
        static bool parseRequest(Buffer &buffer, Request &req, size_t max_body_size);
    
};

#endif
