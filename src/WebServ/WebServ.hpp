#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include <iostream>
#include <cstring>
#include <sstream>
#include <exception>
#include <vector>
#include <map>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <cerrno>
#include <cstdlib>

# define BUFFER_SIZE 4096
# define MAX_CLIENTS 1024
# define SERVER_NAME "WebServ/1.0"
# define DEFAULT_PORT 8080
# define DEFAULT_ROOT "./www"

enum    HttpMethod
{
    GET,
    POST,
    DELETE,
    UNSUPPORTED
};

class   WebServ
{
    private:
        int serverSocket;
        std::string rootDirectory;
        fd_set  masterSet;
        fd_set  readSet;
        int maxFd;
        void    setupServer(int port);
        void    handleNewConnection();
        void    handleClientRequest(int clientSocket);
        HttpMethod  parseHttpMethod(const std::string& method);
        void    sendResponse(int clientSocket, const std::string& response);

    public:
        WebServ();
        ~WebServ();
        void startServer(int port = DEFAULT_PORT, const std::string& root = DEFAULT_ROOT);
};

#endif