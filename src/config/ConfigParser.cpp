#include "ConfigParser.hpp"

static  std::string stripComments(const std::string &line)
{
    size_t  pos = line.find('#');

    if (pos == std::string::npos)
        return (line);
    return (line.substr(0, pos));
}

std::string ConfigParser::trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");

    if (a == std::string::npos)
        return std::string();
    size_t b = s.find_last_not_of(" \t\r\n");

    return s.substr(a, b - a + 1);
}

void    ConfigParser::parseLocationBlock(std::ifstream &file, LocationConfig &loc)
{
    std::string line;

    while (std::getline(file, line))
    {
        line = stripComments(line);
        line = trim(line);

        if (line.empty())
            continue ;
        if (line == "}")
            break ;

        std::istringstream iss(line);
        std::string key;
        iss >> key;
        if (key == "root")
        {
            std::string value;
            iss >> value;
            loc.root = value;
        }
        else if (key == "methods")
        {
            std::string m;
            while (iss >> m) loc.methods.push_back(m);
        }
        else if (key == "directory_listing")
        {
            std::string v;
            iss >> v;
            loc.directory_listing = (v == "on");
        }
        else if (key == "upload_dir")
        {
            std::string v;
            iss >> v;
            loc.upload_dir = v;
        }
        else if (key == "cgi_extension")
        {
            std::string v;
            iss >> v;
            loc.cgi_extension = v;
        }
    }
}

void    ConfigParser::parseServerBlock(std::ifstream &file, ServerConfig &server)
{
    std::string line;

    while (std::getline(file, line))
    {
        line = stripComments(line);
        line = trim(line);

        if (line.empty())
            continue ;
        if (line == "}")
            break ;

        std::istringstream iss(line);
        std::string key;
        iss >> key;
        if (key == "listen")
        {
            std::string v;
            while (iss >> v) server.listen.push_back(v);
        }
        else if (key == "root")
        {
            std::string v; iss >> v;
            LocationConfig loc;
            loc.path = "/";
            loc.root = v;
            server.locations.push_back(loc);
        }
        else if (key == "max_body_size")
        {
            size_t v;
            iss >> v;
            server.max_body_size = v;
        }
        else if (key == "error_page")
        {
            int code; std::string path; iss >> code >> path;
            server.error_pages[code] = path;
        }
        else if (key == "location")
        {
            LocationConfig loc;
            iss >> loc.path;
            parseLocationBlock(file, loc);
            server.locations.push_back(loc);
        }
    }
}

Config  ConfigParser::parseFile(const std::string &filename)
{
    Config  conf;
    std::ifstream file(filename.c_str());

    if (!file.is_open())
    {
        std::cerr << "Could not open config: " << filename << std::endl;
        return (conf);
    }

    std::string line;
    while (std::getline(file, line))
    {
        line = stripComments(line);
        line = trim(line);
        if (line.empty())
            continue ;
        if (line.find("server") != std::string::npos)
        {
            // consume the opening '{' line if it's there
            if (line.find('{') == std::string::npos)
            {
                while (std::getline(file, line))
                {
                    line = trim(line);
                    if (line.find('{') != std::string::npos)
                        break ;
                }
            }
            ServerConfig server;
            parseServerBlock(file, server);
            conf.servers.push_back(server);
        }
    }
    file.close();
    return (conf);
}
