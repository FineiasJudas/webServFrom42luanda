#include "Signal.hpp"
#include <iostream>

static void sigint_handler(int signum)
{
    (void)signum;
    g_running = 0;
    std::cout << "\nSIGINT recebido — encerrando servidor...\n";
}

void    setupSignalHandlers()
{
    signal(SIGINT, sigint_handler);
    // --- Sinais de terminação ---
    signal(SIGQUIT, SIG_IGN);   // Ctrl+\ (quit with core dump)
    signal(SIGTERM, SIG_IGN);   // kill (graceful termination request)
    signal(SIGHUP, SIG_IGN);    // Terminal desconectado (hangup)
    
    // --- Sinais de controle de job ---
    signal(SIGTSTP, SIG_IGN);   // Ctrl+Z (suspend/stop)
    signal(SIGTTIN, SIG_IGN);   // Background process tentou ler do terminal
    signal(SIGTTOU, SIG_IGN);   // Background process tentou escrever no terminal
    signal(SIGCONT, SIG_IGN);   // Continue após SIGSTOP/SIGTSTP
    
    // --- Sinais de I/O ---
    signal(SIGPIPE, SIG_IGN);   // Escrita em pipe/socket fechado (CRÍTICO!)
    signal(SIGIO, SIG_IGN);     // I/O assíncrono disponível
    signal(SIGURG, SIG_IGN);    // Dados urgentes em socket
    
    // --- Sinais de timer/alarme ---
    signal(SIGALRM, SIG_IGN);   // Alarme de timer (alarm())
    signal(SIGVTALRM, SIG_IGN); // Virtual timer (setitimer)
    signal(SIGPROF, SIG_IGN);   // Profiling timer
    
    // --- Sinais definidos pelo usuário ---
    signal(SIGUSR1, SIG_IGN);   // User-defined signal 1
    signal(SIGUSR2, SIG_IGN);   // User-defined signal 2
    
    // --- Sinais de window/terminal ---
    signal(SIGWINCH, SIG_IGN);  // Janela de terminal redimensionada
}

