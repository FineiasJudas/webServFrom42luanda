#include "config/ConfigParser.hpp"
#include "core/Server.hpp"
#include <iostream>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Use: ./webserv <file.config>\n";  
        return (1);
    }

    std::string confFile = argv[1];

    try
    {
        Config  conf = ConfigParser::parseFile(confFile);
        if (conf.servers.empty()) 
        {
            std::cerr << "No server blocks found in config\n";
            return (1);
        }

        // create a Server for each ServerConfig? here we start one Server with first block
        Server server(conf.servers[0]);
        server.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return (0);
}