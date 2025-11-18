#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <map>
#include <string>
#include <vector>
#include <sys/wait.h>
#include "../http/Request.hpp"
#include "../config/Config.hpp"

struct  CgiResult
{
    int     exit_status;
    std::string raw_output;
};

class   CgiHandler
{
    public:
        static CgiResult    execute(const Request& req, 
                const std::string& script_path,
            const ServerConfig& config);

};

#endif
