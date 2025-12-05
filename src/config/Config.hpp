#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "../../includes/Headers.hpp"

/*
struct  LocationConfig
{
    std::string     path;
    std::string     root;
    std::string     cgi_path;
    std::string     upload_dir;
    std::string     cgi_extension;
    std::vector<std::string>    methods;

    bool    directory_listing;
    bool    auto_index_set;
    bool    auto_index;

    int     redirect_code;
    std::string redirect_url;

    LocationConfig()
        : directory_listing(false),
          auto_index_set(false),
          auto_index(false),
          redirect_code(0)
    {}
};
*/
struct CgiConfig {
    std::string cgi_extension;
    std::string cgi_path;
};

struct LocationConfig
{
    std::string     path;
    std::string     root;
   // std::string     cgi_path;
   // std::string     cgi_extension;

    std::string     cgi_path;
    std::string     cgi_extension;
    std::vector<CgiConfig> cgi_configs;

    std::string     upload_dir;
    std::vector<std::string> methods;

    bool    directory_listing;
    bool    auto_index_set;
    bool    auto_index;

    int     redirect_code;
    std::string redirect_url;

    LocationConfig()
        : directory_listing(false),
          auto_index_set(false),
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
    size_t      max_body_size;
    std::string     root;

    bool    auto_index_set;
    bool    auto_index;

    int     cgi_timeout;

    ServerConfig()
        : max_body_size(1024 * 1024),
        root("./examples/www"),
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
