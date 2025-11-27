#include "../../includes/Headers.hpp"
#include "../utils/Utils.hpp"
#include "MasterServer.hpp"
#include "../utils/Logger.hpp"
#include "../http/HttpParser.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../http/Router.hpp"
#include <set>

MasterServer::MasterServer(const std::vector<ServerConfig> &servers) 
    : read_timeout(15), keepalive_timeout(30)
{
    createListenSockets(servers);
}

MasterServer::~MasterServer()
{
    for (size_t i = 0; i < listenSockets.size(); ++i)
        delete listenSockets[i];

    for (std::map<int, Connection*>::iterator it = connections.begin();
         it != connections.end(); ++it)
        delete it->second;
}

void    MasterServer::createListenSockets(const std::vector<ServerConfig> &servers)
{
    std::set<int> createdPorts; // evitar duplicar sockets para mesma porta

    for (size_t i = 0; i < servers.size(); ++i)
    {
        const ServerConfig &srv = servers[i];

        std::string addr_port = servers[i].listen[0];
        size_t sep = addr_port.find(':');
        std::string host = "";
        int port = 0;
        if (sep != std::string::npos)
        {
            host = servers[i].server_names[0];
            port = atoi(addr_port.substr(sep + 1).c_str());
        }
        else
        {
            //host = servers[i].server_names[0].empty() ? "" : servers[i].server_names[0];
            port = atoi(addr_port.c_str());
        }
        if (createdPorts.count(port) == 0)
        {
            ListenSocket *ls = new ListenSocket(host, port);
            listenSockets.push_back(ls);
            poller.addFd(ls->getFd(), EPOLLIN | EPOLLET);
            createdPorts.insert(port);
            listenFdToServer[ls->getFd()] = const_cast<ServerConfig*>(&srv);
            Logger::log(Logger::INFO, "Servidor ouvindo em " + host + ":" + Utils::toString(port));
        }
    }
}

void MasterServer::run()
{
    Logger::log(Logger::INFO, "MasterServer rodando...");

    while (true)
    {
        std::vector<epoll_event> events = poller.wait(1000);

        for (size_t i = 0; i < events.size(); ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            bool isListen = false;
            for (size_t j = 0; j < listenSockets.size(); ++j)
                if (fd == listenSockets[j]->getFd())
                {
                    isListen = true;
                    break;
                }

            if (isListen)
                handleAccept(fd);

            else if (ev & EPOLLIN)
                handleRead(fd);

            else if (ev & EPOLLOUT)
                handleWrite(fd);

            else if (ev & (EPOLLHUP | EPOLLERR))
                closeConnection(fd);
        }

        checkWriteTimeouts();
    }
}

void MasterServer::handleAccept(int listenFd)
{
    int     flags;
    int     client_fd;
    socklen_t   len;
    struct sockaddr_in  cli;

    len = sizeof(cli);
    client_fd = accept(listenFd, (struct sockaddr*)&cli, &len);
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

void MasterServer::handleRead(int clientFd)
{
    ssize_t     n;
    char    buf[8192];
    Connection  *conn = connections[clientFd];

    if (!conn)
    {
        closeConnection(clientFd);
        return ;
    }
    // 📨 1) LER DADOS DO SOCKET
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
            closeConnection(clientFd);
            return ;
        }
    }

     //🧪 DEBUG
    Logger::log(Logger::INFO, "📨 Dados recebidos na conexão FD "
              + Utils::toString(clientFd) + ": "
              + Utils::toString(conn->getInputBuffer().size()) + " bytes acumulados");

    // 🔍 2) PROCESSAR requests COMPLETOS
    int iteration = 0;
    while (HttpParser::hasCompleteRequest(conn->getInputBuffer()))
    {
        Request     req;

        if (!HttpParser::parseRequest(conn->getInputBuffer(), req, 
            listenFdToServer[listenFdToServer.begin()->first]->max_body_size))
        {
            Response    res;
            res.status = 400;
            res.body = "<h1>400 Bad Request</h1><a href=\"/\">Voltar</a>";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            res.headers["Content-Type"] = "text/html";

            conn->getOutputBuffer().append(res.toString());
            poller.modifyFd(clientFd, EPOLLOUT | EPOLLET);
            Logger::log(Logger::ERROR, "Status "
                  + Utils::toString(res.status) + " " + Response::reasonPhrase(res.status));
            break ;
        }
        if (req.too_large_body)
        {
            Response    res;

            res.status = 413;
            res.body = "<h1>413 Payload Too Large</h1><a href=\"/\">Voltar</a>";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            res.headers["Content-Type"] = "text/html";

            conn->getOutputBuffer().append(res.toString());
            poller.modifyFd(clientFd, EPOLLOUT | EPOLLET);
            Logger::log(Logger::ERROR, "Status "
                  + Utils::toString(res.status) + " " + Response::reasonPhrase(res.status));
            break ;
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
                  + Utils::toString(clientFd));

        // 🧠 Roteamento
        Response res = Router::route(req, *listenFdToServer[listenFdToServer.begin()->first]);

        res.status >= 200 && res.status <= 400 ? Logger::log(Logger::DEBUG, "Status "
                  + Utils::toString(res.status) + " "
                  + Response::reasonPhrase(res.status)) : res.status >= 500 ? Logger::log(Logger::WARN, "Status "
                  + Utils::toString(res.status) + " "
                  + Response::reasonPhrase(res.status)) : Logger::log(Logger::ERROR, "Status "
                  + Utils::toString(res.status) + " "
                  + Response::reasonPhrase(res.status));

        // Enviar resposta
        conn->getOutputBuffer().append(res.toString());
        poller.modifyFd(clientFd, EPOLLOUT | EPOLLET);

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

void    MasterServer::handleWrite(int clientFd)
{
    Connection  *conn = connections[clientFd];
    if (!conn)
    {
        closeConnection(clientFd);
        return ;
    }

    // Pegar a resposta do buffer de saída, 
    // e enviar o conteúdo de volta ao cliente
    Buffer &out = conn->getOutputBuffer();
    if (out.empty())
    { 
        closeConnection(clientFd); 
        return ;
    }
    
    ssize_t sent = conn->writeToFd(out.data(), out.size());
    if (sent > 0)
        out.consume(sent);

    if (conn->shouldCloseAfterSend())
        closeConnection(clientFd);
    else
        poller.modifyFd(clientFd, EPOLLOUT | EPOLLET); // esperar próxima request (keep-alive)*/
}

void MasterServer::closeConnection(int clientFd)
{
    poller.removeFd(clientFd);
    if (connections.count(clientFd))
    {
        delete  connections[clientFd];
        connections.erase(clientFd);
    }
    close(clientFd);
    Logger::log(Logger::WARN, "Conexão fechada FD " + Utils::toString(clientFd));
}

void MasterServer::checkWriteTimeouts()
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
