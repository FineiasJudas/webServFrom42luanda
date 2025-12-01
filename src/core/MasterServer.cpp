#include "../../includes/Headers.hpp"
#include "../utils/Utils.hpp"
#include "MasterServer.hpp"
#include "../utils/Logger.hpp"
#include "../http/HttpParser.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../http/Router.hpp"
#include "Signal.hpp"
#include <set>

volatile bool   g_running = true;

MasterServer::MasterServer(const std::vector<ServerConfig> &servers)
{
    read_timeout = 15;
    write_timeout = 15;
    keepalive_timeout = 20;

    createListenSockets(servers);
}

MasterServer::~MasterServer()
{
    for (std::map<int, Connection*>::iterator it = connections.begin();
         it != connections.end(); ++it)
    {
        delete it->second;
    }
}

void MasterServer::createListenSockets(const std::vector<ServerConfig>& servers)
{
    for (size_t i = 0; i < servers.size(); ++i)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);

        fcntl(fd, F_SETFL, O_NONBLOCK);

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(atoi(servers[i].listen[0].c_str()));
        addr.sin_addr.s_addr = INADDR_ANY;

        bind(fd, (sockaddr*)&addr, sizeof(addr));
        listen(fd, 10);

        listenFds.push_back(fd);
        listenFdToServer[fd].push_back((ServerConfig*)&servers[i]);

        poller.addFd(fd, EPOLLIN | EPOLLET);
        Logger::log(Logger::INFO, "Listening on port " +
                   (servers[i].listen[0]));
    }
}

void    MasterServer::handleAccept(int listenFd)
{
    while (true)
    {
        socklen_t   len;
        struct sockaddr_in  cli;

        len = sizeof(cli);
        int clientFd = accept(listenFd, (struct sockaddr *)&cli, &len);
        if (clientFd < 0)
            return ;

        fcntl(clientFd, F_SETFL, O_NONBLOCK);

        Connection* conn = new Connection(clientFd);
        conn->setListenFd(listenFd);

        connections[clientFd] = conn;

        poller.addFd(clientFd, EPOLLIN | EPOLLET);

        Logger::log(Logger::INFO, "Nova conexão aceita FD "
            + Utils::toString(clientFd));
    }
}

ServerConfig    *MasterServer::selectServer(const Request& req, int listenFd)
{
    const std::vector<ServerConfig *> &list = listenFdToServer[listenFd];

    if (list.empty())
        return NULL;

    std::string host = req.headers.count("Host") ? req.headers.at("Host") : "";
    size_t pos = host.find(':');
    if (pos != std::string::npos)
        host = host.substr(0, pos);

    for (size_t i = 0; i < list.size(); i++)
    {
        for (size_t j = 0; j < list[i]->server_names.size(); j++)
        {
            if (list[i]->server_names[j] == host)
                return list[i];
        }
    }
    return list[0];
}

bool MasterServer::isListenFd(int fd) const
{
    return listenFdToServer.count(fd) > 0;
}

void    MasterServer::handleRead(int clientFd)
{
    Connection  *conn = connections[clientFd];

    if (!conn)
        return ;

    while (true)
    {
        ssize_t n = conn->readFromFd();
        if (n <= 0)
            break;
    }

    Request req;
    while (HttpParser::hasCompleteRequest(conn->getInputBuffer()))
    {
        if (!HttpParser::parseRequest(conn->getInputBuffer(), req, 8192))
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
            poller.modifyFd(clientFd, EPOLLOUT | EPOLLET);
            Logger::log(Logger::ERROR, "Status "
                  + Utils::toString(res.status) + " " + Response::reasonPhrase(res.status));
            break ;
        }

        conn->setServer(selectServer(req, conn->getListenFd()));
        ServerConfig* sc = conn->getServer();

        Logger::log(Logger::INFO, req.method + " " + req.uri + " " + req.version + " recebido na FD "
                  + Utils::toString(clientFd));

        Response res = Router::route(req, *sc);

        // 🧠 Roteamento

        res.status >= 200 && res.status <= 400 ? Logger::log(Logger::DEBUG, "Status "
                  + Utils::toString(res.status) + " "
                  + Response::reasonPhrase(res.status)) : res.status >= 500 ? Logger::log(Logger::WARN, "Status "
                  + Utils::toString(res.status) + " "
                  + Response::reasonPhrase(res.status)) : Logger::log(Logger::ERROR, "Status "
                  + Utils::toString(res.status) + " "
                  + Response::reasonPhrase(res.status));

        conn->getOutputBuffer().append(res.toString());
        poller.modifyFd(clientFd, EPOLLOUT | EPOLLET);

        if (req.headers.count("Connection") && req.headers.at("Connection") == "close")
            conn->setCloseAfterSend(true);
    }
}

// Oleekeikke
void    MasterServer::handleWrite(int clientFd)
{
    Connection *conn = connections[clientFd];
    if (!conn)
    {
        closeConnection(clientFd);
        return ;
    }

    conn->last_activity_time = time(NULL);

    Buffer &out = conn->getOutputBuffer();

    // nada para enviar → voltar a ler
    if (out.empty())
    {
        conn->waiting_for_write = false;
        conn->last_activity_time = time(NULL);
        poller.modifyFd(clientFd, EPOLLIN | EPOLLET);
        return ;
    }

    // marca início do write
    if (!conn->waiting_for_write)
    {
        conn->waiting_for_write = true;
        conn->write_start_time = time(NULL);
    }

    // escreve
    ssize_t sent = conn->writeToFd(out.data(), out.size());

    if (sent > 0)
        out.consume(sent);

    if (out.empty())
    {
        // terminou de escrever
        conn->waiting_for_write = false;

        if (conn->shouldCloseAfterSend())
        {
            closeConnection(clientFd);
            return ;
        }

        // volta para modo leitura (HTTP keep-alive)
        poller.modifyFd(clientFd, EPOLLIN | EPOLLET);
        return;
    }

    // ainda há dados → mantém EPOLLOUT
    poller.modifyFd(clientFd, EPOLLOUT | EPOLLET);
}

/*void    MasterServer::handleWrite(int clientFd)
{
    Connection *conn = connections[clientFd];
    if (!conn)
    {
        closeConnection(clientFd);
        return ;
    }

    conn->last_activity_time = time(NULL);

    Buffer &out = conn->getOutputBuffer();

    // nada para enviar → voltar a ler
    if (out.empty())
    {
        conn->waiting_for_write = false;
        poller.modifyFd(clientFd, EPOLLIN | EPOLLET);
        return ;
    }

    // marca início do write
    if (!conn->waiting_for_write)
    {
        conn->waiting_for_write = true;
        conn->write_start_time = time(NULL);
    }

    // escreve
    ssize_t sent = conn->writeToFd(out.data(), out.size());

    if (sent > 0)
        out.consume(sent);

    if (out.empty())
    {
        // terminou de escrever
        conn->waiting_for_write = false;

        if (conn->shouldCloseAfterSend())
        {
            closeConnection(clientFd);
            return ;
        }

        // volta para modo leitura (HTTP keep-alive)
        poller.modifyFd(clientFd, EPOLLIN | EPOLLET);
        return;
    }

    // ainda há dados → mantém EPOLLOUT
    poller.modifyFd(clientFd, EPOLLOUT | EPOLLET);
}*/


void    MasterServer::closeConnection(int clientFd)
{
    poller.removeFd(clientFd);
    ::close(clientFd);
    delete connections[clientFd];
    connections.erase(clientFd);
    Logger::log(Logger::WARN, "Conexão fechada FD " + Utils::toString(clientFd));
}

void    MasterServer::checkTimeouts()
{
    time_t  now = time(NULL);

    for (std::map<int, Connection*>::iterator it = connections.begin();
         it != connections.end(); )
    {
        Connection *c = it->second;

        long    idle = now - c->last_activity_time;

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

        if (c->waiting_for_write && idle > write_timeout)
        {
            // WRITE TIMEOUT (ex: 10–15s)
            Logger::log(Logger::WARN, "[TIMEOUT] WRITE Timeout FD " + Utils::toString(c->getFd()));
            poller.removeFd(c->getFd());
            ::close(c->getFd());
            delete c;
            connections.erase(it++);
            continue;
        }

        ++it;
    }
}

void    MasterServer::run()
{
    Logger::log(Logger::INFO, "MasterServer iniciado...");
    while (g_running)
    {
        std::vector<PollEvent> events = poller.waitEvents(1000);

        for (size_t i = 0; i < events.size(); i++)
        {
            int fd = events[i].fd;

            if (!g_running)
                break ;

            if (events[i].isError())
            {
                closeConnection(fd);
                continue;
            }

            if (isListenFd(fd))
            {
                handleAccept(fd);
                continue;
            }

            if (events[i].isReadable())
                handleRead(fd);

            if (events[i].isWritable())
                handleWrite(fd);
        }

        checkTimeouts();
    }
    Logger::log(Logger::INFO, "MasterServer encerrando conexões...");

    // Fechar todas as conexões abertas
    for (std::map<int, Connection*>::iterator it = connections.begin();
         it != connections.end(); ++it)
    {
        poller.removeFd(it->first);
        ::close(it->first);
        delete it->second;
    }

    connections.clear();

    Logger::log(Logger::INFO, "MasterServer finalizado.");
}

/*void    MasterServer::checkWriteTimeouts()
{
    long    now = time(NULL);

    for (std::map<int, Connection*>::iterator it = connections.begin();
         it != connections.end(); )
    {
        Connection *c = it->second;

        long    idle = now - c->last_activity_time;

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

        if (c->waiting_for_write && idle > write_timeout)
        {
            // WRITE TIMEOUT (ex: 10–15s)
            Logger::log(Logger::WARN, "[TIMEOUT] WRITE Timeout FD " + Utils::toString(c->getFd()));
            poller.removeFd(c->getFd());
            ::close(c->getFd());
            delete c;
            connections.erase(it++);
            continue;
        }

        ++it;
    }
}*/