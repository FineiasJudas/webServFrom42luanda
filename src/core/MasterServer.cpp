#include "MasterServer.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"
#include "../http/HttpParser.hpp"
#include "../http/Router.hpp"
#include "../../includes/Headers.hpp"
#include "../exceptions/WebServException.hpp"
#include "./../utils/keywords.hpp"
#include "../session/SessionManager.hpp"

volatile sig_atomic_t   g_running = 1;

MasterServer::MasterServer(const std::vector<ServerConfig> &servers)
{
    this->read_timeout = 15;
    this->write_timeout = 15;
    this->keepalive_timeout = 20;

    createListenSockets(servers);
    if (listenFdToServers.empty())
        throw PortException("Portas inválidas ou nenhum ServerConfig criado");
}

MasterServer::~MasterServer()
{
    for (std::map<int, Connection *>::iterator it = connections.begin();
         it != connections.end(); ++it)
    {
        int fd = it->first;
        close(fd);
        delete it->second;
    }

    for (std::map<int, ServerConfig *>::iterator it = listenFdToServers.begin();
         it != listenFdToServers.end(); ++it)
        close(it->first);
}

int MasterServer::parsePortFromListenString(const std::string &s) const
{
    std::string portstr;
    size_t colon = s.find(':');

    if (colon == std::string::npos)
        portstr = s;
    else
        portstr = s.substr(colon + 1);
    if (portstr.empty() || !Utils::isNumber(portstr))
        return (-1);
    return std::atoi(portstr.c_str());
}

void    MasterServer::createListenSockets(const std::vector<ServerConfig> &servers)
{
    int     fd;
    int     port;

    for (size_t i = 0; i < servers.size(); ++i)
    {
        const ServerConfig  &sc = servers[i];

        if (sc.listen.size() > 0)
        {
            port = parsePortFromListenString(sc.listen[0]);
            if (port < KW::MIN_VALUE_PORT || port > KW::MAX_VALUE_PORT)
            {
                std::ostringstream ss;
                ss << "Porta inválida: Uma porta precisa ser um número entre " << KW::MIN_VALUE_PORT << " e " << KW::MAX_VALUE_PORT;
                Logger::log(Logger::ERROR, ss.str());

                std::ostringstream ss2;
                ss2 << "Porta inválida: porta inserida: " << sc.listen[0];
                Logger::log(Logger::ERROR, ss2.str());

                continue;
            }
            if (std::find(ports.begin(), ports.end(), port) == ports.end())
            {
                ports.push_back(port);
                fd = createListenSocketForPort(port);
                if (fd >= 0)
                {
                    listenFdToServers[fd] = ((ServerConfig *)&sc);
                    Logger::log(Logger::INFO, "Ouvindo na porta " + Utils::toString(port));
                }
                else
                    Logger::log(Logger::ERROR, "Falha ao criar um listen para a porta " + Utils::toString(port));
            }
            else
            {
                Logger::log(Logger::ERROR, "Porta " + Utils::toString(port) + " já ocupada por um Server");
                continue;
            }
        }
    }
}

int MasterServer::createListenSocketForPort(int port)
{
    int fd;
    int yes = 1;
    int flags;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        Logger::log(Logger::ERROR, "socket() fail: " + Utils::toString(errno));
        return (-1);
    }

    yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        flags = 0;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in  addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        Logger::log(Logger::ERROR, "bind() fail: " + Utils::toString(errno));
        close(fd);
        return (-1);
    }

    if (listen(fd, MAX_EVENTS) < 0)
    {
        Logger::log(Logger::ERROR, "listen() fail: " + Utils::toString(errno));
        close(fd);
        return (-1);
    }
    poller.addFd(fd, EPOLLIN);
    return (fd);
}

bool MasterServer::isListenFd(int fd) const
{
    return (listenFdToServers.count(fd) > 0);
}

ServerConfig *MasterServer::selectServerForRequest(const Request &req, int listenFd)
{
    (void)req;
    std::map<int, ServerConfig *>::const_iterator it = listenFdToServers.find(listenFd);
    if (it == listenFdToServers.end())
        return (NULL);

    ServerConfig *defaultServer = it->second;
    if (!defaultServer)
        return (NULL);
    return (defaultServer);
}

void    MasterServer::handleAccept(int listenFd)
{
    int     flags;
    int     clientFd;

    clientFd = accept(listenFd, NULL, NULL);
    if (clientFd < 0)
        return;

    flags = fcntl(clientFd, F_GETFL, 0);
    if (flags < 0)
        flags = 0;
    fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

    Connection *c = new Connection(clientFd);
    c->setListenFd(listenFd);
    c->last_activity_time = time(NULL);
    c->waiting_for_write = false;
    c->write_start_time = 0;
    connections[clientFd] = c;

    poller.addFd(clientFd, EPOLLIN);

    Logger::log(Logger::NEW, "Nova conexão aceita FD " + Utils::toString(clientFd));
}

/*
subject página 09

1 • Você nunca deve fazer uma operação de leitura ou escrita sem passar por poll()
(ou equivalente).

2 • Verificar o valor de errno para ajustar o comportamento do servidor é estritamente
proibido após realizar uma operação de leitura ou escrita.

3 • Você não é obrigado a usar poll() (ou equivalente) antes de read() para recuperar
seu arquivo de configuração.

===============  parece que já estamos a comprir com apenas o pontos acima ===============
*/

void    MasterServer::handleRead(int clientFd)
{
    std::map<int, Connection *>::iterator it = connections.find(clientFd);
    if (it == connections.end())
        return ;

    size_t max_body = 1024 * 1024;
    Connection *conn = it->second;
    std::map<int, ServerConfig *>::const_iterator vec = listenFdToServers.find(conn->getListenFd());
    if (vec != listenFdToServers.end() && vec->second)
        max_body = vec->second->max_body_size;

    //Ler UMA vez (epoll LT)
    ssize_t n = conn->readFromFd();
    if (conn->getInputBuffer().size() > max_body)
    {
        Response    res;

        res.status = 413;
        res.body = "<h1>413 Payload Too Large</h1><a href=\"/\">Voltar</a>";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        res.headers["Content-Type"] = "text/html";
        conn->getOutputBuffer().append(res.toString());
        poller.modifyFd(clientFd, EPOLLOUT | EPOLLET);
        Logger::log(Logger::NEW, "Status " + Utils::toString(res.status) + 
            " " + Response::reasonPhrase(res.status));
        conn->getInputBuffer().clear();
        return ;
    }
    if (n == 0)
    {
        closeConnection(clientFd);
        return ;
    }
    if (n < 0)
    {
        closeConnection(clientFd);
        return ;
    }//nada para ler agora idiota e nem precisas diferenciar erros

    //Processar requisições completas
    int processed = 0;
    while (HttpParser::hasCompleteRequest(conn->getInputBuffer()))
    {
        Request     req;

        bool ok = HttpParser::parseRequest(conn->getInputBuffer(), req, max_body);
        if (!ok)
           break ;
        // determinar servidor real para este pedido
        ServerConfig *sc = selectServerForRequest(req, conn->getListenFd());
        conn->setServer(sc);

        std::string connHdr = "";
        if (req.headers.count("Connection"))
            connHdr = req.headers.at("Connection");
        if (connHdr == "close")
            conn->setCloseAfterSend(true);
        else if (req.version == "HTTP/1.0" && connHdr != "keep-alive")
            conn->setCloseAfterSend(true);
        else
            conn->setCloseAfterSend(false);

        Logger::log(Logger::INFO, req.method + " " + req.uri + 
            " " + req.version + " recebido na FD " + Utils::toString(clientFd));

        // Roteamento
        Response res = Router::route(req, *sc);

        res.status >= 200 && res.status <= 300 ? Logger::log(Logger::DEBUG, "Status " + Utils::toString(res.status) + 
        " " + Response::reasonPhrase(res.status)) : res.status >= 400 && res.status <= 500 ? 
            Logger::log(Logger::ERROR, "Status " + Utils::toString(res.status) + " " + Response::reasonPhrase(res.status))
                : Logger::log(Logger::WINT, "Status " + Utils::toString(res.status) + " " + Response::reasonPhrase(res.status));

        conn->getOutputBuffer().append(res.toString());
        conn->getInputBuffer().clear();
        processed++;
    }

    if (processed > 0)
        poller.modifyFd(clientFd, EPOLLOUT);
}

void    MasterServer::handleWrite(int clientFd)
{
    std::map<int, Connection *>::iterator it = connections.find(clientFd);
    if (it == connections.end())
        return;

    Connection *conn = it->second;
    Buffer &out = conn->getOutputBuffer();

    if (out.empty())// nada para escrever
    {
        conn->waiting_for_write = false;
        poller.modifyFd(clientFd, EPOLLIN);
        return;
    }

    if (!conn->waiting_for_write)
    {
        conn->waiting_for_write = true;
        conn->write_start_time = time(NULL);
    }

    ssize_t sent = conn->writeToFd(out.data(), out.size());

    if (sent > 0){out.consume(sent);}
    else if (sent == 0){return;}
    else{return;}

    if (conn->shouldCloseAfterSend() && out.empty())
    {return closeConnection(clientFd);}

    if (out.empty())//terminou de escrever
    {
        conn->waiting_for_write = false;
        poller.modifyFd(clientFd, EPOLLIN);
    }
    else {poller.modifyFd(clientFd, EPOLLOUT);}// continuar a escutar escrita
}



void    MasterServer::closeConnection(int clientFd)
{
    std::map<int, Connection *>::iterator it = connections.find(clientFd);

    if (it == connections.end())
        return ;

    poller.removeFd(clientFd);
    ::close(clientFd);
    delete it->second;
    connections.erase(it);
    Logger::log(Logger::WARN, "Conexão fechada FD " + Utils::toString(clientFd));
}

void    MasterServer::checkTimeouts()
{
    time_t  idle;
    time_t  now = time(NULL);

    std::vector<int> toClose;

    for (std::map<int, Connection *>::iterator it = connections.begin(); it != connections.end(); ++it)
    {
        Connection *c = it->second;
        idle = now - c->last_activity_time;
        if (!c->getInputBuffer().empty() && idle > read_timeout)
        {
            Logger::log(Logger::WARN, "[TIMEOUT] Read Timeout FD " + Utils::toString(c->getFd()));
            toClose.push_back(c->getFd());
            continue;
        }
        if (c->waiting_for_write && (now - c->write_start_time) > write_timeout)
        {
            Logger::log(Logger::WARN, "[TIMEOUT] WRITE Timeout FD " + Utils::toString(c->getFd()));
            toClose.push_back(c->getFd());
            continue;
        }
        if (!c->waiting_for_write && c->getInputBuffer().empty() && idle > keepalive_timeout)
        {
            Logger::log(Logger::WARN, "[TIMEOUT] Keep-Alive Timeout FD " + Utils::toString(c->getFd()));
            toClose.push_back(c->getFd());
            continue;
        }
    }

    for (size_t i = 0; i < toClose.size(); ++i)
        closeConnection(toClose[i]);
}

void    MasterServer::run()
{
    int     fd;

    Logger::log(Logger::INFO, "MasterServer iniciado...");
    while (g_running)
    {
        std::vector<PollEvent> events = poller.waitEvents(1000);

        if (!g_running)
            break;

        for (size_t i = 0; i < events.size(); ++i)
        {
            fd = events[i].fd;

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
        g_sessions.cleanup();
    }
    Logger::log(Logger::WINT, "MasterServer encerrando conexões...");
    for (std::map<int, Connection *>::iterator it = connections.begin();
         it != connections.end(); ++it)
    {
        poller.removeFd(it->first);
        ::close(it->first);
        delete it->second;
    }
    connections.clear();
    Logger::log(Logger::WINT, "MasterServer finalizado.");
}