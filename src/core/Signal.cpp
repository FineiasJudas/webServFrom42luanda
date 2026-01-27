#include "Signal.hpp"
#include "../utils/Logger.hpp"
#include <iostream>

static void sigint_handler(int signum)
{
    (void)signum;
    if (signum == SIGINT)
        {std::cout << "\nSIGINT recebido — encerrando servidor...\n";}
    else if (signum == SIGTERM)
        {std::cout << "\nSIGTERM recebido — encerrando servidor...\n";}
    else if (signum == SIGQUIT)
        {std::cout << "\nSIGQUIT recebido — encerrando servidor...\n";}
    g_running = 0;
}

void setupSignalHandlers()
{

    // Encerramento limpo
    /*
     SIGINT  — Ctrl+C no terminal
     SIGTERM — pedido de encerramento (kill)
     SIGQUIT — Ctrl+\ no terminal
    */
    signal(SIGTERM, sigint_handler);
    signal(SIGINT, sigint_handler);
    signal(SIGQUIT, sigint_handler); // rever o subject ou a folha de avaliação

    /*
     O kernel envia SIGPIPE automaticamente quando:
     Um processo tenta escrever (write, send) em um pipe ou socket
     que já foi fechado pelo outro lado (ex: navegador fechou a conexão).
    */
    signal(SIGPIPE, SIG_IGN);
}
