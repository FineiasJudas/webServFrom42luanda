#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "../../includes/Headers.hpp"
#include "../config/Config.hpp"
#include "Request.hpp"
#include "Response.hpp"

class   Router
{
    public:
        static Response route(const Request &req, const ServerConfig &config);
    
};

#endif
