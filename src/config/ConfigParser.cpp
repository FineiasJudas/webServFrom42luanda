#include "ConfigParser.hpp"
#include "./../utils/keywords.hpp"

static  std::string stripComments(const std::string &line)
{
    size_t  pos = line.find(KW::COMMENT); //'#');

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
        if (line == KW::BLOCK_END) //if (line == "}")
            break ;

        std::istringstream  iss(line);
        std::string key;

        iss >> key;
        if (key == KW::ROOT) //if (key == "root")
            iss >> loc.root;
        else if (key == KW::METHODS) //else if (key == "methods")
        {
            std::string m;
            while (iss >> m) {
                loc.methods.push_back(m);
                //std::cout << "method " << m << " add" << std::endl;
                //###### nao verificamos se os metodos sao validos antes de add
            }
        }
        else if (key == KW::DIRECTORY_LISTING) //else if (key == "directory_listing")
        {
            std::string v; iss >> v;
            loc.directory_listing = (v == KW::ON); //(v == "on");
        }
        else if (key == KW::AUTO_INDEX) //else if (key == "auto_index")
        {
            std::string v; iss >> v;
            loc.auto_index = (v == KW::ON); //(v == "on");
            loc.auto_index_set = true;
        }
        else if (key == KW::UPLOAD_DIR) //else if (key == "upload_dir")
            iss >> loc.upload_dir;
        else if (key == KW::CGI_EXTENSION) //else if (key == "cgi_extension")
            iss >> loc.cgi_extension;
        else if (key == KW::CGI_PATH) //else if (key == "cgi_path")
        {
            std::string v;
            iss >> v;
            loc.cgi_path = v;
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
        if (line == KW::BLOCK_END) //if (line == "}")
            break ;

        std::istringstream iss(line);
        std::string     key;

        iss >> key;
        if (key == KW::LISTEN) //if (key == "listen")
        {
            std::string v;
            while (iss >> v)
                server.listen.push_back(v);
        }
        else if (key == KW::SERVER_NAME) //else if (key == "server_name")
        {
            std::string v;
            while (iss >> v)
                server.server_names.push_back(v);
        }
        else if (key == KW::AUTO_INDEX) //else if (key == "auto_index")
        {
            std::string v; iss >> v;
            server.auto_index = (v == KW::ON); //(v == "on");
            server.auto_index_set = true;
        }
        else if (key == KW::ROOT) //else if (key == "root")
            // Agora root pertence ao ServerConfig
            iss >> server.root;
        else if (key == KW::MAX_BODY_SIZE) //else if (key == "max_body_size")
            iss >> server.max_body_size;
        else if (key == KW::ERROR_PAGE) //else if (key == "error_page")
        {
            int     code;
            std::string path;
            iss >> code >> path;
            server.error_pages[code] = path;
        }
        else if (key == KW::LOCATION) //else if (key == "location")
        {
            LocationConfig  loc;
            iss >> loc.path;
            parseLocationBlock(file, loc);
            server.locations.push_back(loc);
        }
    }

    // ===== GARANTI QUE "/" EXISTE COMO LOCATION DEFAULT =====
    bool    hasRootLocation = false;
    for (size_t i = 0; i < server.locations.size(); i++)
        if (server.locations[i].path == KW::ROOT_DEFAULT) //"/")
            hasRootLocation = true;

    if (!hasRootLocation)
    {
        LocationConfig  loc;
    
        loc.path = KW::ROOT_DEFAULT;// "/";
        loc.root = server.root; // root do servidor
        //######  os metodos como get e post
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

        if (line == KW::SERVER)
        {
            // Skip '{'
            while (std::getline(file, line))
            {
                line = stripComments(line);
                line = trim(line);
                if (line == KW::BLOCK_START)
                    break ;
            }

            ServerConfig    server;
            parseServerBlock(file, server);

            conf.servers.push_back(server);
        }
    }
    return (conf);
}
