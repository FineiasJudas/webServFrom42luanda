#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <signal.h>

extern volatile bool    g_running;

void    setupSignalHandlers();

#endif
