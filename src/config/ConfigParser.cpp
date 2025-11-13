#include "ConfigParser.hpp"

// Lê a próxima palavra ignorando espaços e quebras de linha
std::string ConfigParser::nextToken(std::istringstream &iss)
{
    std::string token;
    iss >> token;
    return token;
}

// ------------------------------------------------------------
// Lê um bloco de "location { ... }"
// ------------------------------------------------------------
void ConfigParser::parseLocation(std::ifstream &file, LocationConfig &loc)
{
    std::string line;
    while (std::getline(file, line))
    {
        if (line.find('}') != std::string::npos)
            break;
        std::istringstream iss(line);
        std::string key = nextToken(iss);

        if (key == "root")
            iss >> loc.root;
        else if (key == "methods")
        {
            std::string method;
            while (iss >> method)
                loc.methods.push_back(method);
        }
        else if (key == "directory_listing")
        {
            std::string value;
            iss >> value;
            loc.directory_listing = (value == "on");
        }
        else if (key == "upload_dir")
            iss >> loc.upload_dir;
        else if (key == "cgi_extension")
            iss >> loc.cgi_extension;
    }
}

// ------------------------------------------------------------
// Lê um bloco "server { ... }"
// ------------------------------------------------------------
void ConfigParser::parseServer(std::ifstream &file, ServerConfig &server)
{
    std::string line;
    while (std::getline(file, line))
    {
        if (line.find('}') != std::string::npos)
            break;

        std::istringstream iss(line);
        std::string key = nextToken(iss);

        if (key == "listen")
        {
            std::string value;
            while (iss >> value)
                server.listen.push_back(value);
        }
        else if (key == "root")
        {
            LocationConfig loc;
            loc.path = "/";
            iss >> loc.root;
            server.locations.push_back(loc);
        }
        else if (key == "max_body_size")
        {
            iss >> server.max_body_size;
        }
        else if (key == "error_page")
        {
            int code;
            std::string path;
            iss >> code >> path;
            server.error_pages[code] = path;
        }
        else if (key == "location")
        {
            LocationConfig loc;
            iss >> loc.path;
            parseLocation(file, loc);
            server.locations.push_back(loc);
        }
    }
}

// ------------------------------------------------------------
// Função principal: lê todo o ficheiro de configuração
// ------------------------------------------------------------
Config ConfigParser::parseFile(const std::string &filename)
{
    Config conf;
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        std::cerr << "Erro: não foi possível abrir " << filename << std::endl;
        return conf;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.find("server") != std::string::npos)
        {
            ServerConfig server;
            parseServer(file, server);
            conf.servers.push_back(server);
        }
    }

    file.close();
    return conf;
}
