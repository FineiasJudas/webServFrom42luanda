#include "ConfigParser.hpp"
#include "./../utils/keywords.hpp"
#include "./../utils/Utils.hpp"
#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>

// =========================
// Funções utilitárias
// =========================
static std::string trim(const std::string &s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace(s[start]))
        start++;

    size_t end = s.size();
    while (end > start && std::isspace(s[end - 1]))
        end--;

    return s.substr(start, end - start);
}

static bool endsWithSemicolon(const std::string &s)
{
    return !s.empty() && s[s.size() - 1] == ';';
}

static std::string stripSemicolon(const std::string &s)
{
    if (endsWithSemicolon(s))
        return trim(s.substr(0, s.size() - 1));
    return s;
}

// =========================
// Parser de Location
// =========================
void    ConfigParser::parseLocation(std::ifstream &file,
                                 LocationConfig &loc,
                                 const std::string &firstLine)
{
    std::string     path;
    std::istringstream iss(firstLine);
    std::string     tmp;

    iss >> tmp;     // location
    iss >> path;    // /abc
    loc.path = path;

    std::string line;
    while (std::getline(file, line))
    {
        line = trim(line);
        if (line == "}")
            break ;

        if (line.empty())
            continue ;

        // Remove ponto e vírgula
        line = stripSemicolon(line);

        std::istringstream iss(line);
        std::string key, value;
        iss >> key;

        if (key == "root")
        {
            iss >> value;
            loc.root = value;
        }
        else if (key == "index")
        {
            iss >> value;
            loc.index = value;
        }
        else if (key == "methods")
        {
            while (iss >> value)
                loc.methods.push_back(value);
        }
        else if (key == "auto_index")
        {
            iss >> value;
            loc.auto_index = (value == "on");
            loc.auto_index_set = true;
        }
        else if (key == "upload_dir")
        {
            iss >> value;
            loc.upload_dir = value;
        }
        else if (key == "return")
        {
            iss >> value;
            loc.redirect_code = std::atoi(value.c_str());
            iss >> loc.redirect_url;
        }
        else if (key == "cgi")
        {
            CgiConfig   cgiConfig;
            iss >> cgiConfig.extension;
            iss >> cgiConfig.path;
            loc.cgi.push_back(cgiConfig);
        }
    }
}

// =========================
// Parser de Server
// =========================
void    ConfigParser::parseServer(std::ifstream &file, ServerConfig &cfg)
{
    std::string     line;
    std::string     pendingCgiExt; // extensão ainda sem path

    while (std::getline(file, line))
    {
        line = trim(line);
        if (line == "}" || line == "};")
            break ;

        if (line.empty())
            continue ;

        // Remove ;
        line = stripSemicolon(line);

        std::istringstream iss(line);
        std::string key, value;
        iss >> key;

        if (key == "listen")
        {
            iss >> value;
            cfg.listen.push_back(value.c_str());
        }
        else if (key == "server_name")
        {
            while (iss >> value)
                cfg.server_names.push_back(value);
        }
        else if (key == "root")
        {
            iss >> cfg.root;
        }
        else if (key == "max_body_size")
        {
            iss >> value;
            cfg.max_body_size = std::atoi(value.c_str());
        }
        else if (key == "auto_index")
        {
            iss >> value;
            cfg.auto_index = (value == "on");
            cfg.auto_index_set = true;
        }
        else if (key == "error_page")
        {
            int code;
            iss >> code;
            iss >> value;
            cfg.error_pages[code] = value;
        }
        else if (key == "cgi_timeout")
        {
            iss >> value;
            cfg.cgi_timeout = std::atoi(value.c_str());
        }
        else if (key == "location")
        {
            LocationConfig loc;
            parseLocation(file, loc, line);
            cfg.locations.push_back(loc);
        }
    }
}

// =========================
// Parse Geral
// =========================
Config ConfigParser::parseFile(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file: " + filename);

    Config config;
    std::string line;

    while (std::getline(file, line))
    {
        line = trim(line);
        std::stringstream ss(line);
        std::string keyword;

        ss >> keyword;
        if (keyword.empty() || keyword[0] == '#')
            continue ;

        if (keyword == "server" || keyword == "server{")
        {
            ServerConfig    srv;
            parseServer(file, srv);
            config.servers.push_back(srv);
        }
    }

    return config;
}
