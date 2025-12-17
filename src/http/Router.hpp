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

    private:
        static bool handleSession(const Request &req, Response &res);
        static bool handleSessionGeneric(const Request &req, Response &res);
        static bool handleCsrf(const Request &req, Response &res);
        static bool handleLogin(const Request &req, Response &res);
        static bool handleLogout(const Request &req, Response &res);
        static LocationConfig   findLocConfig(const std::string &uri, const ServerConfig &config);

};


#endif
