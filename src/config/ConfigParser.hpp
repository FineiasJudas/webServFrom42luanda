#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Config.hpp"

class   ConfigParser
{
    public:
        static Config parseFile(const std::string &filename);

    private:
        static void parseServer(std::ifstream &file, ServerConfig &server);
        static void parseLocation(std::ifstream &file, LocationConfig &loc);
        static std::string nextToken(std::istringstream &iss);
};

#endif
