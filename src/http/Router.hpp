#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "../config/Config.hpp"
#include <string>

class   Router
{
    public:
        // Função principal: recebe um Request e devolve um Response
        static Response route(const Request &req, const ServerConfig &config);
};

#endif
