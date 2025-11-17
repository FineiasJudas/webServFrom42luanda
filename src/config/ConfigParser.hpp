#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "../../includes/Headers.hpp"
#include "Config.hpp"

class ConfigParser {
public:
    static Config parseFile(const std::string &filename);

private:
    static std::string trim(const std::string &s);
    static void parseServerBlock(std::ifstream &file, ServerConfig &server);
    static void parseLocationBlock(std::ifstream &file, LocationConfig &loc);
};

#endif
