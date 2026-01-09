#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "../../includes/Headers.hpp"

struct  CgiConfig
{
    std::string extension;
    std::string path;
};

struct  LocationConfig
{
    std::string path;
    std::string root;
    std::string index;

    std::vector<CgiConfig> cgi;

    std::string upload_dir;
    std::vector<std::string> methods;

    bool auto_index_set;
    bool auto_index;

    int redirect_code;
    std::string redirect_url;

    LocationConfig()
        : auto_index_set(false),
          auto_index(false),
          redirect_code(0)
    {}
};

struct  ServerConfig
{
    std::vector<std::string>    listen;
    std::vector<std::string>    server_names;
    std::map<int, std::string>  error_pages;
    std::vector<LocationConfig> locations;
    size_t  max_body_size;
    std::string root;
    bool    auto_index_set;
    bool    auto_index;
    int     cgi_timeout;

    ServerConfig()
        : max_body_size(1 * (1024 * 1024)),
          root("./examples/www/site1"),
          auto_index_set(false),
          auto_index(false),
          cgi_timeout(3)
    {}
};

class   Config
{
    public:
        std::vector<ServerConfig>   servers;
};

#endif
