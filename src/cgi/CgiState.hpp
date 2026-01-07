// CgiState.hpp
#ifndef CGISTATE_HPP
#define CGISTATE_HPP

#include <string>
#include <ctime>

struct CgiState
{
    pid_t pid;
    int pipe_fd;          // fd de leitura do stdout do CGI (stdout_pipe[0])
    std::string output;   // dados já lidos do CGI
    time_t start_time;
    int exit_status;
    bool active;

    CgiState() : pid(-1), pipe_fd(-1), exit_status(-1), start_time(0), active(false) {}
};

#endif