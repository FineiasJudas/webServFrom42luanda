#include "WebServ/WebServ.hpp"

int main(void)
{
    WebServ server;
    server.startServer(8080, "./www");
    return (0);
}
