#include "WebServ.hpp"

WebServ::WebServ() : serverSocket(-1), rootDirectory(DEFAULT_ROOT), maxFd(0)
{
    FD_ZERO(&masterSet);
    FD_ZERO(&readSet);
}

WebServ::~WebServ()
{
    if (serverSocket != -1)
        close(serverSocket);
}

void WebServ::startServer(int port, const std::string& root)
{
    rootDirectory = root;
    setupServer(port);
    std::cout << "Servidor iniciado na porta " << port << " com raiz em " << rootDirectory << std::endl;

    while (true)
    {
        readSet = masterSet;
        if (select(maxFd + 1, &readSet, NULL, NULL, NULL) == -1)
        {
            std::cerr << "Erro no select: " << strerror(errno) << std::endl;
            continue ;
        }

        for (int i = 0; i <= maxFd; ++i)
        {
            if (FD_ISSET(i, &readSet))
            {
                if (i == serverSocket)
                    handleNewConnection();
                else
                    handleClientRequest(i);
            }
        }
    }
}

void    WebServ::setupServer(int port)
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        throw std::runtime_error("Erro ao criar o socket do servidor: " + std::string(strerror(errno)));
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        throw std::runtime_error("Erro ao configurar o socket do servidor: " + std::string(strerror(errno)));
    }

    struct sockaddr_in  serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
    {
        throw std::runtime_error("Erro ao vincular o socket do servidor: " + std::string(strerror(errno)));
    }

    if (listen(serverSocket, SOMAXCONN) == -1)
    {
        throw std::runtime_error("Erro ao escutar no socket do servidor: " + std::string(strerror(errno)));
    }

    FD_SET(serverSocket, &masterSet);
    maxFd = serverSocket;
}

void WebServ::handleNewConnection()
{
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
    if (clientSocket == -1)
    {
        std::cerr << "Erro ao aceitar nova conexao: " << strerror(errno) << std::endl;
        return;
    }

    FD_SET(clientSocket, &masterSet);
    if (clientSocket > maxFd)
        maxFd = clientSocket;

    std::cout << "Nova conexao aceita: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
}

void WebServ::handleClientRequest(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesRead <= 0)
    {
        if (bytesRead == 0)
            std::cout << "Cliente desconectado: socket " << clientSocket << std::endl;
        else
            std::cerr << "Erro ao receber dados do cliente: " << strerror(errno) << std::endl;

        close(clientSocket);
        FD_CLR(clientSocket, &masterSet);
        return;
    }

    std::string request(buffer);
    std::istringstream requestStream(request);
    std::string methodStr;
    requestStream >> methodStr;

    HttpMethod method = parseHttpMethod(methodStr);
    if (method == UNSUPPORTED)
    {
        sendResponse(clientSocket, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
        return;
    }

    // Aqui você pode adicionar o processamento adicional do pedido HTTP

    sendResponse(clientSocket, "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!");
}

HttpMethod WebServ::parseHttpMethod(const std::string& method)
{
    if (method == "GET")
        return GET;
    else if (method == "POST")
        return POST;
    else if (method == "DELETE")
        return DELETE;
    else
        return UNSUPPORTED;
}

void WebServ::sendResponse(int clientSocket, const std::string &response)
{
    ssize_t bytesSent = send(clientSocket, response.c_str(), response.length(), 0);
    if (bytesSent == -1)
    {
        std::cerr << "Erro ao enviar resposta ao cliente: " << strerror(errno) << std::endl;
    }
}
