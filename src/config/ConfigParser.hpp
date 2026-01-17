#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "../../includes/Headers.hpp"
#include "Config.hpp"

class   ConfigParser
{
    public:
        static Config   parseFile(const std::string &filename);

    private:
        static void     parseLocation(std::ifstream &file, LocationConfig &loc,
                                 const std::string &firstLine);
        static void     parseServer(std::ifstream &file, ServerConfig &cfg);

        static void     serverConfigFixer(ServerConfig &cfg);

};

#endif
