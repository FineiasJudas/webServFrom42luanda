#include "core/Server.hpp"
#include "config/ConfigParser.hpp"

int main(int argc, char **argv) {
    std::string conf = "conf/default.conf";
    if (argc > 1)
        conf = argv[1];

    try {
        Config config = ConfigParser::parseFile(conf);
        Server server;
        server.init(config);
        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
