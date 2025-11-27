#include "Server.hpp"
#include "../http/HttpParser.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../http/Router.hpp"
#include "../utils/Utils.hpp"
#include "../utils/Logger.hpp"
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
    Logger::log(Logger::INFO, "Servidor com " + Utils::toString(listenSockets.size()) + " sockets ouvindo");
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
    Logger::log(Logger::INFO, "Servidor rodando...");
    while (true)
    {
        std::vector<struct epoll_event> events = poller.wait(1000);
        for (size_t i = 0; i < events.size(); ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            bool    isListen = false;
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
    Logger::log(Logger::INFO, "Nova conexão: FD " + Utils::toString(client_fd));
}

void    Server::handleRead(int conn_fd)
{
    ssize_t     n;
    char    buf[8192];
    Connection  *conn = connections[conn_fd];

    if (!conn)
    {
        closeConnection(conn_fd);
        return ;
    }
    // 🔁 1) Ler até EAGAIN (non-blocking)
    while (true)
    {
        n = conn->readFromFd(buf, sizeof(buf));
        if (n > 0)
            continue ;

        // sem mais dados por agora → normal!
        if (n == -1)
            break ;

        if (n <= 0)
        {
            closeConnection(conn_fd);
            return ;
        }
    }

    /* 🧪 DEBUG
    Logger::log(Logger::INFO, "📨 Dados recebidos na conexão FD "
              + Utils::toString( conn_fd) + ": "
              + Utils::toString(conn->getInputBuffer().size()) + " bytes acumulados");*/

    // 🔍 2) PROCESSAR requests COMPLETOS
    int iteration = 0;
    while (HttpParser::hasCompleteRequest(conn->getInputBuffer()))
    {
        Request     req;

        if (!HttpParser::parseRequest(conn->getInputBuffer(), req, config.max_body_size))
        {
            Response    res;
            res.status = 400;
            res.body = "<h1>400 Bad Request</h1><a href=\"/\">Voltar</a>";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            res.headers["Content-Type"] = "text/html";

            conn->getOutputBuffer().append(res.toString());
            poller.modifyFd(conn_fd, EPOLLOUT | EPOLLET);
            return ;
        }
        if (req.too_large_body)
        {
            Response    res;

            res.status = 413;
            res.body = "<h1>413 Payload Too Large</h1><a href=\"/\">Voltar</a>";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            res.headers["Content-Type"] = "text/html";

            conn->getOutputBuffer().append(res.toString());
            poller.modifyFd(conn_fd, EPOLLOUT | EPOLLET);
            return ;
        }

        // Verificar o Keep-alive / Close
        if (req.headers["Connection"] == "close")
            conn->setCloseAfterSend(true);
        else if (req.version == "HTTP/1.0" &&
                 req.headers["Connection"] != "keep-alive")
            conn->setCloseAfterSend(true);
        else
            conn->setCloseAfterSend(false);

        Logger::log(Logger::INFO, req.method + " " + req.uri + " " + req.version + " recebido na FD "
                  + Utils::toString(conn_fd));

        // 🧠 Roteamento
        Response res = Router::route(req, config);

        res.status >= 200 && res.status <= 400 ? Logger::log(Logger::DEBUG, "Status "
                  + Utils::toString(res.status) + " "
                  + Response::reasonPhrase(res.status)) : res.status >= 500 ? Logger::log(Logger::WARN, "Status "
                  + Utils::toString(res.status) + " "
                  + Response::reasonPhrase(res.status)) : Logger::log(Logger::ERROR, "Status "
                  + Utils::toString(res.status) + " "
                  + Response::reasonPhrase(res.status));

        // Enviar resposta
        conn->getOutputBuffer().append(res.toString());
        poller.modifyFd(conn_fd, EPOLLOUT | EPOLLET);

        iteration++;
    }

    /*if (iteration == 0)
        Logger::log(Logger::INFO, "🔄 Nenhum request completo processado na FD "
                  + Utils::toString(conn_fd));
    else
        Logger::log(Logger::INFO, "✅ " + Utils::toString(iteration)
                  + " request(s) processado(s) na FD "
                  + Utils::toString(conn_fd));*/
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
    
    ssize_t sent = conn->writeToFd(out.data(), out.size());
    if (sent > 0)
        out.consume(sent);

    if (conn->shouldCloseAfterSend())
        closeConnection(conn_fd);
    else
        poller.modifyFd(conn_fd, EPOLLOUT | EPOLLET); // esperar próxima request (keep-alive)*/
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
    Logger::log(Logger::WARN, "Conexão fechada: FD " + Utils::toString(conn_fd));
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
            Logger::log(Logger::WARN, "[TIMEOUT] Read Timeout FD " + Utils::toString(c->getFd()));
            poller.removeFd(c->getFd());
            ::close(c->getFd());
            delete c;
            connections.erase(it++);
            continue ;
        }

        if (c->getInputBuffer().empty() && idle > keepalive_timeout)
        {
            // KEEP-ALIVE TIMEOUT
            Logger::log(Logger::WARN, "[TIMEOUT] Keep-Alive Timeout FD " + Utils::toString(c->getFd()));
            poller.removeFd(c->getFd());
            ::close(c->getFd());
            delete c;
            connections.erase(it++);
            continue ;
        }
        ++it;
    }
}

