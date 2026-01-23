#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <signal.h>

extern volatile sig_atomic_t g_running;  // ← Tipo correto

void setupSignalHandlers();

#endif