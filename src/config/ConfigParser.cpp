#include "ConfigParser.hpp"

static  std::string stripComments(const std::string &line)
{
    size_t  pos = line.find('#');

    if (pos == std::string::npos)
        return (line);
    return (line.substr(0, pos));
}

// TRIM
std::string ConfigParser::trim(const std::string &s)
{
    size_t  start = s.find_first_not_of(" \t\r\n");
    size_t  end   = s.find_last_not_of(" \t\r\n");

    if (start == std::string::npos)
        return ("");
    return s.substr(start, end - start + 1);
}

// LOCATION BLOCK
void ConfigParser::parseLocationBlock(std::ifstream &file, LocationConfig &loc)
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

        std::istringstream  iss(line);
        std::string key;

        iss >> key;
        if (key == "root")
            iss >> loc.root;
        else if (key == "methods")
        {
            std::string m;
            while (iss >> m) loc.methods.push_back(m);
        }
        else if (key == "directory_listing")
        {
            std::string v; iss >> v;
            loc.directory_listing = (v == "on");
        }
        else if (key == "auto_index")
        {
            std::string v; iss >> v;
            loc.auto_index = (v == "on");
            loc.auto_index_set = true;
        }
        else if (key == "upload_dir")
            iss >> loc.upload_dir;
        else if (key == "cgi_extension")
            iss >> loc.cgi_extension;
        else if (key == "cgi_path")
        {
            std::string v;
            iss >> v;
            loc.cgi_path = v;
        }
        else if (key == "return")
        {
            int code;
            std::string url;
            iss >> code >> url;

            loc.redirect_code = code;
            loc.redirect_url = url;
        }
    }
}

// SERVER BLOCK
void    ConfigParser::parseServerBlock(std::ifstream &file, ServerConfig &server)
{
    std::string     line;

    while (std::getline(file, line))
    {
        line = stripComments(line);
        line = trim(line);
        if (line.empty())
            continue ;
        if (line == "}")
            break ;

        std::istringstream iss(line);
        std::string     key;

        iss >> key;
        if (key == "listen")
        {
            std::string v;
            while (iss >> v)
                server.listen.push_back(v);
        }
        else if (key == "server_name")
        {
            std::string v;
            while (iss >> v)
                server.server_names.push_back(v);
        }
        else if (key == "auto_index")
        {
            std::string v; iss >> v;
            server.auto_index = (v == "on");
            server.auto_index_set = true;
        }
        else if (key == "root")
            // Agora root pertence ao ServerConfig
            iss >> server.root;
        else if (key == "max_body_size")
            iss >> server.max_body_size;
        else if (key == "error_page")
        {
            int     code;
            std::string path;
            iss >> code >> path;
            server.error_pages[code] = path;
        }
        else if (key == "location")
        {
            LocationConfig  loc;
            iss >> loc.path;
            parseLocationBlock(file, loc);
            server.locations.push_back(loc);
        }
        else if (key == "cgi_timeout")
        {
            int v; iss >> v;
            server.cgi_timeout = v;
        }
    }

    // ===== GARANTI QUE "/" EXISTE COMO LOCATION DEFAULT =====
    bool    hasRootLocation = false;
    for (size_t i = 0; i < server.locations.size(); i++)
        if (server.locations[i].path == "/")
            hasRootLocation = true;

    if (!hasRootLocation)
    {
        LocationConfig  loc;
    
        loc.path = "/";
        loc.root = server.root; // root do servidor
        server.locations.push_back(loc);
    }
}

// MAIN PARSING FUNCTION
Config  ConfigParser::parseFile(const std::string &filename)
{
    Config  conf;
    std::ifstream   file(filename.c_str());

    if (!file.is_open())
    {
        std::cerr << "Could not open config: " << filename << std::endl;
        return (conf);
    }

    std::string     line;
    while (std::getline(file, line))
    {
        line = stripComments(line);
        line = trim(line);
        if (line.empty())
            continue ;

        if (line == "server")
        {
            // Skip '{'
            while (std::getline(file, line))
            {
                line = stripComments(line);
                line = trim(line);
                if (line == "{")
                    break ;
            }

            ServerConfig    server;
            parseServerBlock(file, server);

            conf.servers.push_back(server);
        }
    }
    return (conf);
}
