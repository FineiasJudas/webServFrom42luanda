#include "config/ConfigParser.hpp"
#include "core/Server.hpp"
#include <iostream>

int main(int argc, char **argv)
{
    std::string configFile = "conf/default.conf";
    if (argc > 1)
        configFile = argv[1];

    Config conf = ConfigParser::parseFile(configFile);
    if (conf.servers.empty()) {
        std::cerr << "Nenhum servidor configurado!\n";
        return 1;
    }

    Server server(conf.servers[0]);
    server.run();
    return 0;
}
