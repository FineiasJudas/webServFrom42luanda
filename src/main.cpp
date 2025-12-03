#include "config/ConfigParser.hpp"
#include "core/MasterServer.hpp"
#include "utils/Logger.hpp"
#include <iostream>

int main(int ac, char **av)
{
    if (ac != 2)
    {
        std::cerr << "Use: ./webserv <file.config>\n";
        return 1;
    }

    std::string confFile = av[1];

    try
    {
        Config conf = ConfigParser::parseFile(confFile); //ok

        if (conf.servers.empty())
        {
            std::cerr << "Nenhum server encontrado\n";
            return 1;
        }

        Logger::init(Logger::DEBUG, "");

        MasterServer master(conf.servers);
        master.run();

        Logger::shutdown();
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }

    return 0;
}
