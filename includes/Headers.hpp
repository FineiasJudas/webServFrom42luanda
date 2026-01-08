#ifndef HEADERS_HPP
#define HEADERS_HPP

/* C/C++ std headers */
#include <iostream>
#include <string>
#include <vector> 
#include <map> 
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>

/* POSIX / sockets / epoll */
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_EVENTS 128

enum CgiType
{
    CGI_PHP,
    CGI_PYTHON,
    CGI_UNKNOWN
};

#endif
