#include "config/ConfigParser.hpp"
#include "core/Server.hpp"
#include "utils/Logger.hpp"
#include <iostream>

int main(int ac, char **av)
{
    if (ac != 2)
    {
        std::cerr << "Use: ./webserv <file.config>\n";  
        return (1);
    }


    std::string confFile = av[1];

    try
    {
        Config  conf = ConfigParser::parseFile(confFile);
        if (conf.servers.empty()) 
        {
            std::cerr << "No server blocks found in config\n";
            return (1);
        }

        Logger::init(Logger::DEBUG, "");
        // create a Server for each ServerConfig? here we start one Server with first block
        Server  server(conf.servers[0]);
        server.run();
        Logger::shutdown();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return (0);
}