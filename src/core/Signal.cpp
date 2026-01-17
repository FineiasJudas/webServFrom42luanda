#include "Signal.hpp"
#include <iostream>

static void sigint_handler(int signum)
{
    if (signum == SIGINT)
    {
        g_running = 0;
        std::cout << "\nSIGINT recebido — encerrando servidor...\n";
    }
}

void setupSignalHandlers()
{
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
}

