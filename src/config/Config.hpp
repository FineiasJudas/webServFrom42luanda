#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct  LocationConfig
{
    std::string path;
    std::string root;
    std::vector<std::string> methods;
    bool    directory_listing;
    std::string upload_dir;
    std::string cgi_extension; // ex ".php"
};

struct  ServerConfig
{
    std::vector<std::string> listen; // "0.0.0.0:8080" ou ":8080"
    std::map<int, std::string>  error_pages; // 404 -> "/errors/404.html"
    size_t  max_body_size;
    std::vector<LocationConfig> locations;
};

class   Config
{
    public:
        std::vector<ServerConfig>   servers;
};

#endif
