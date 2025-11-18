#include "Server.hpp"
#include "../http/HttpParser.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../http/Router.hpp"
#include "../utils/Utils.hpp"
#include <iostream>
#include <cstring>

Server::Server(const ServerConfig &conf)
    : config(conf), read_timeout(15), keepalive_timeout(30)
{
    // Criar sockets e colocar em modo Listen
    for (size_t i = 0; i < conf.listen.size(); ++i)
    {
        std::string entry = conf.listen[i];
        std::string host = "";
        int port = 0;
        size_t colon = entry.find(':');
        if (colon != std::string::npos)
        {
            host = entry.substr(0, colon);
            port = atoi(entry.substr(colon + 1).c_str());
        }
        else
            port = atoi(entry.c_str());
        if (host == "")
            host = "0.0.0.0";

        ListenSocket *ls = new ListenSocket(host, port);
        listenSockets.push_back(ls);
        poller.addFd(ls->getFd(),  EPOLLIN | EPOLLET);
    }
    std::cout << "Servidor com " << listenSockets.size() << " sockets ouvindo\n";
}

Server::~Server()
{
    for (size_t i = 0; i < listenSockets.size(); ++i)
        delete (listenSockets[i]);
    for (std::map<int, Connection *>::iterator it = connections.begin();
         it != connections.end(); ++it)
        delete (it->second);
}

void    Server::run()
{
    std::cout << "Servidor rodando...\n";
    while (true)
    {
        std::vector<struct epoll_event> events = poller.wait(1000);
        for (size_t i = 0; i < events.size(); ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            bool isListen = false;
            for (size_t j = 0; j < listenSockets.size(); ++j)
                if (fd == listenSockets[j]->getFd())
                {
                    isListen = true;
                    break ;
                }

            if (isListen)
                handleAccept(fd);
            else 
            {
                if (ev & EPOLLIN)
                    handleRead(fd);
                else if (ev & EPOLLOUT)
                    handleWrite(fd);
                else if (ev & (EPOLLHUP | EPOLLERR))
                    closeConnection(fd);
            }
        }
        checkWriteTimeouts();
    }
}

void    Server::handleAccept(int listen_fd)
{
    int     flags;
    int     client_fd;
    socklen_t   len;
    struct sockaddr_in  cli;

    len = sizeof(cli);
    client_fd = accept(listen_fd, (struct sockaddr*)&cli, &len);
    if (client_fd < 0)
        return ;

    // Setar a conexão como não bloqueante
    flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    // Adicionar a conexão aos fds monitorados pelo Poller
    poller.addFd(client_fd, EPOLLIN | EPOLLET);
    connections[client_fd] = new Connection(client_fd);
    std::cout << "\n🧩 Nova conexão: FD " << client_fd << std::endl;
}

void    Server::handleRead(int conn_fd)
{
    ssize_t     n;

    Connection *conn = connections[conn_fd];
    if (!conn)
    {
        closeConnection(conn_fd);
        return ;
    }

    char buf[8192];

    // Ler o conteúdo do enviado pelo cliente
    n = conn->readFromFd(buf, sizeof(buf));
    if (n <= 0)
    {
        closeConnection(conn_fd);
        return ;
    }

    // Tentar fazer o parse da requisição (possivelmente múltiplas)
    while (HttpParser::hasCompleteRequest(conn->getInputBuffer()))
    {
        Request     req;

        if (!HttpParser::parseRequest(conn->getInputBuffer(), req, config.max_body_size))
            break ;
        
        // Log da requisição
        std::cout << "\n======== Request ========" << std::endl;
        std::cout << "Method: " << req.method << std::endl;
        std::cout << "Version: " << req.version << std::endl;
        std::cout << "Body: " << req.body << std::endl;
        std::cout << "Connection: " << req.headers["Connection"] << std::endl;

        // Checar o tipo de conexão (Close/Keep-alive)
        if (req.headers["Connection"] == "close")
            conn->setCloseAfterSend(true);
        else if (req.version == "HTTP/1.0" &&
                req.headers["Connection"] != "keep-alive")
            conn->setCloseAfterSend(true);
        else
            conn->setCloseAfterSend(false);

        // Criar resposta
        Response res = Router::route(req, config);

         // Log da resposta
        std::cout << "\n======== Response ========" << std::endl;
        std::cout << "Status: " << res.status << std::endl;
        std::cout << "Content-Type: " << res.headers["Content-Type"] << std::endl;
        std::cout << "Connection: " << res.headers["Connection"] << std::endl;
        std::cout << "Content-Length: " << res.headers["Content-Length"] << std::endl;

        // Converter a resposta em string e adicionar no buffer de saída
        std::string out = res.toString();
        conn->getOutputBuffer().append(out);

        // Mudar o fd da conexão para escrita
        poller.modifyFd(conn_fd, EPOLLOUT | EPOLLET);
    }
}

void    Server::handleWrite(int conn_fd)
{
    Connection  *conn = connections[conn_fd];
    if (!conn)
    {
        closeConnection(conn_fd);
        return ;
    }

    // Pegar a resposta do buffer de saída, 
    // e enviar o conteúdo de volta ao cliente
    Buffer &out = conn->getOutputBuffer();
    if (out.empty())
    {
        closeConnection(conn_fd); 
        return ;
    }
    
    const std::vector<char> &data = out.getData();
    ssize_t sent = ::write(conn_fd, &data[0], data.size());
    if (sent > 0)
        out.consume(sent);

    if (conn->shouldCloseAfterSend())
        closeConnection(conn_fd);
    else
        poller.modifyFd(conn_fd, EPOLLOUT); // esperar próxima request (keep-alive)*/
}

void    Server::closeConnection(int conn_fd)
{
    poller.removeFd(conn_fd);
    if (connections.count(conn_fd))
    {
        delete  connections[conn_fd];
        connections.erase(conn_fd);
    }
    close(conn_fd);
    std::cout << "\n🔒 Conexão fechada FD " << conn_fd << std::endl;
}

void    Server::checkWriteTimeouts()
{
    long    now = time(NULL);

    for (std::map<int, Connection*>::iterator it = connections.begin();
         it != connections.end(); )
    {
        Connection *c = it->second;

        long idle = now - c->last_activity_time;

        if (!c->getInputBuffer().empty() && idle > read_timeout)
        {
            // READ TIMEOUT
            std::cout << "\n[TIMEOUT] Read Timeout FD " << c->getFd() << std::endl;
            poller.removeFd(c->getFd());
            ::close(c->getFd());
            delete c;
            connections.erase(it++);
            continue;
        }

        if (c->getInputBuffer().empty() && idle > keepalive_timeout)
        {
            // KEEP-ALIVE TIMEOUT
            std::cout << "\n[TIMEOUT] Keep-alive Timeout FD " << c->getFd() << std::endl;
            poller.removeFd(c->getFd());
            ::close(c->getFd());
            delete c;
            connections.erase(it++);
            continue;
        }

        ++it;
    }
}

