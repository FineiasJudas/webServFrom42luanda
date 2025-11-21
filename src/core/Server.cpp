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
    Connection  *conn = connections[conn_fd];
    if (!conn)
    {
        closeConnection(conn_fd);
        return ;
    }

    char    buf[8192];
    ssize_t     n;

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

    // 🧪 DEBUG
    std::cout << "\n📨 Dados recebidos na conexão FD "
              << conn_fd << ": "
              << conn->getInputBuffer().size() << " bytes acumulados\n";

    // 🔍 2) PROCESSAR requests COMPLETOS
    int iteration = 0;
    while (HttpParser::hasCompleteRequest(conn->getInputBuffer()))
    {
        Request     req;
        std::cout << "\n🔍 Fazendo parse da requisição FD " << conn_fd << std::endl;

        if (!HttpParser::parseRequest(conn->getInputBuffer(), req, config.max_body_size))
        {
            Response    res;
            res.status = 400;
            res.body = "<h1>400 Bad Request</h1>";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            res.headers["Content-Type"] = "text/html";

            conn->getOutputBuffer().append(res.toString());
            poller.modifyFd(conn_fd, EPOLLOUT | EPOLLET);
            return ;
        }

        // 🔌 Keep-alive / Close
        if (req.headers["Connection"] == "close")
            conn->setCloseAfterSend(true);
        else if (req.version == "HTTP/1.0" &&
                 req.headers["Connection"] != "keep-alive")
            conn->setCloseAfterSend(true);
        else
            conn->setCloseAfterSend(false);

        std::cout << "\n\n====== ✅ Requisição parseada com sucesso =======\n"
                  << req.method << " " << req.uri << " " << req.version << "\n\n";

        // 🧠 Roteamento
        Response res = Router::route(req, config);

        std::cout << "\n\n======= ➡️  Resposta gerada ========\nStatus: "
                  << res.status << " "
                  << Response::reasonPhrase(res.status) << "\n\n";

        // Enviar resposta
        conn->getOutputBuffer().append(res.toString());
        poller.modifyFd(conn_fd, EPOLLOUT | EPOLLET);

        iteration++;
    }

    if (iteration == 0)
        std::cout << "⏳ Nenhum request completo ainda\n";
    else
        std::cout << "🔄 " << iteration << " request(s) processado(s)\n";
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

