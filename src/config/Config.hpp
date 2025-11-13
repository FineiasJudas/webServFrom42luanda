#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct LocationConfig {
    std::string path;
    std::string root;
    std::vector<std::string> methods;
    bool directory_listing;
    std::string upload_dir;
    std::string cgi_extension;
};

struct ServerConfig {
    std::vector<std::string> listen;
    std::map<int, std::string> error_pages;
    size_t max_body_size;
    std::vector<LocationConfig> locations;
};

class Config {
public:
    std::vector<ServerConfig> servers;
};

#endif
