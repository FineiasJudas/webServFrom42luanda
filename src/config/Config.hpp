#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "../../includes/Headers.hpp"

struct  LocationConfig
{
    std::string     path;
    std::string     root;
    std::vector<std::string>    methods;
    bool    directory_listing;
    std::string     upload_dir;
    std::string     cgi_extension;

    LocationConfig() : directory_listing(false) {}
};

struct  ServerConfig
{
    std::vector<std::string>    listen;
    std::map<int, std::string>  error_pages;
    size_t      max_body_size;
    std::vector<LocationConfig> locations;

    ServerConfig() : max_body_size(1024 * 1024) {}
};

class   Config
{
    public:
        std::vector<ServerConfig> servers;
};

#endif
