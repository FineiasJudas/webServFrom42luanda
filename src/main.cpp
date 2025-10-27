#include "../includes/Headers.hpp"

int main(void)
{
    Server  server("0.0.0.0", 8080);
    server.start();
    return (0);
}