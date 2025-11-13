#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "../../includes/Headers.hpp"
#include "Config.hpp"

class   ConfigParser
{
    public:
        static Config parseFile(const std::string &path);
};

#endif
