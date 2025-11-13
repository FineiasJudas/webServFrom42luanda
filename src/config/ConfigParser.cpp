#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>

Config  ConfigParser::parseFile(const std::string &path)
{
    Config config;
    std::ifstream file(path.c_str());
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file: " + path);

    std::string line;
    ServerConfig current_server;
    LocationConfig current_location;
    bool in_server = false;
    bool in_location = false;

    while (std::getline(file, line)) {
        // remove espaços iniciais e finais
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream ss(line);
        std::string key, value;
        ss >> key;

        if (key == "server") {
            in_server = true;
            current_server = ServerConfig();
        } else if (key == "location") {
            in_location = true;
            ss >> current_location.path;
        } else if (key == "listen") {
            ss >> value;
            current_server.listen.push_back(value);
        } else if (key == "root") {
            ss >> value;
            if (in_location)
                current_location.root = value;
            else {
                // cria location default para o server root
                current_location.path = "/";
                current_location.root = value;
                current_server.locations.push_back(current_location);
                current_location = LocationConfig();
            }
        } else if (key == "error_page") {
            int code;
            std::string file_path;
            ss >> code >> file_path;
            current_server.error_pages[code] = file_path;
        } else if (key == "max_body_size") {
            ss >> value;
            current_server.max_body_size = std::atoi(value.c_str());
        } else if (key == "}") {
            if (in_location) {
                current_server.locations.push_back(current_location);
                current_location = LocationConfig();
                in_location = false;
            } else if (in_server) {
                config.servers.push_back(current_server);
                current_server = ServerConfig();
                in_server = false;
            }
        }
    }
    return config;
}
