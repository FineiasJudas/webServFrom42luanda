#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "../../includes/Headers.hpp"

struct LocationConfig
{
    std::string path;
    std::string root;
    std::vector<std::string>    methods;
    bool    directory_listing;
    std::string cgi_path;
    std::string upload_dir;
    std::string cgi_extension;
    bool    auto_index_set;       // novo
    bool    auto_index;           // novo

    LocationConfig()
        : directory_listing(false),
          auto_index_set(false),
          auto_index(false)
    {}
};

struct ServerConfig
{
    std::vector<std::string>    listen;
    std::vector<std::string>    server_names;
    std::map<int, std::string>  error_pages;
    size_t  max_body_size;
    std::vector<LocationConfig> locations;
    std::string root;                            // root padrão do server

    bool    auto_index_set;       // novo
    bool    auto_index;           // novo

    ServerConfig()
        : max_body_size(1024 * 1024),
            root("./examples/www"),
          auto_index_set(false),
          auto_index(false)
    {}
};

class   Config
{
    public:
        std::vector<ServerConfig>   servers;
};

#endif
