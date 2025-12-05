#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <map>
#include <string>
#include <vector>
#include "../http/Request.hpp"
#include "../config/Config.hpp"
#include "../../includes/Headers.hpp"
#include "../http/Response.hpp"

struct  CgiResult
{
    int exit_status;
    std::string raw_output;
};

class   CgiHandler
{
    public:
        static CgiResult execute(
            const Request &req,
                              const std::string &script_path,
                              const ServerConfig &config,
                              const LocationConfig &loc,
                              const CgiConfig &cgiConfig);
                              
        static Response handleCgiRequest(const Request &req,
                                  const ServerConfig &config,
                                  const LocationConfig &loc,
                                  const CgiConfig &cgiConfig);
        static Response parseCgiOutput(const std::string &raw);
};

#endif
