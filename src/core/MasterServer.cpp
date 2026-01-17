#include "MasterServer.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"
#include "../http/HttpParser.hpp"
#include "../http/Router.hpp"
#include "../../includes/Headers.hpp"
#include <netdb.h>
#include "../exceptions/WebServException.hpp"
#include "./../utils/keywords.hpp"
#include "../session/SessionManager.hpp"
#include "../cgi/CgiHandler.hpp"

volatile sig_atomic_t g_running = 1;

MasterServer::MasterServer(std::vector<ServerConfig> &servers)
{
    this->read_timeout = 30;
    this->write_timeout = 30;
    this->keepalive_timeout = 60;

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

void    MasterServer::createListenSockets(std::vector<ServerConfig> &servers)
{
    for (size_t i = 0; i < servers.size(); ++i)
    {
        ServerConfig &server = servers[i];

        // 1. Obter listen string
        std::string listen = server.listen[0]; // ex: "8080" ou "127.0.0.1:8080"

        if (listen.empty())
        {
            Logger::log(Logger::ERROR, "Server sem diretiva listen");
            continue ;
        }

        // 2. Normalizar listenKey
        std::string ip = "0.0.0.0";
        std::string port;

        size_t colon = listen.find(':');
        if (colon == std::string::npos)
            port = listen;
        else
        {
            ip   = listen.substr(0, colon);
            port = listen.substr(colon + 1);
        }

        std::string listenKey = ip + ":" + port;

        // 3. Evitar duplicação
        if (usedListenKeys.count(listenKey))
        {
            Logger::log(Logger::ERROR,
                "Porta duplicada ignorada: " + listenKey);
            continue ;
        }

        // 4. Criar socket
        int fd = createListenSocketForPort(listen);
        if (fd < 0)
        {
            Logger::log(Logger::ERROR,
                "Falha ao criar socket para " + listenKey);
            continue ;
        }

        // 5. Associar fd → server
        listenFdToServers[fd] = &server;
        usedListenKeys.insert(listenKey);

        Logger::log(Logger::INFO,
            "Servidor escutando em " + listenKey);
    }
}


int MasterServer::createListenSocketForPort(const std::string &_listen)
{
    std::string ip = "0.0.0.0";
    std::string port;

    size_t colon = _listen.find(':');
    if (colon == std::string::npos)
        port = _listen;
    else
    {
        ip   = _listen.substr(0, colon);
        port = _listen.substr(colon + 1);
    }

    struct addrinfo hints;
    struct addrinfo *res;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    int ret = getaddrinfo(
        ip == "0.0.0.0" ? NULL : ip.c_str(),
        port.c_str(),
        &hints,
        &res
    );

    if (ret != 0)
    {
        Logger::log(Logger::ERROR,
            "getaddrinfo failed for " + _listen + ": " + gai_strerror(ret));
        return (-1);
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0)
    {
        Logger::log(Logger::ERROR, "socket() failed");
        freeaddrinfo(res);
        return (-1);
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        Logger::log(Logger::ERROR, "setsockopt(SO_REUSEADDR) failed");
        close(fd);
        freeaddrinfo(res);
        return (-1);
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    {
        Logger::log(Logger::ERROR, "fcntl(O_NONBLOCK) failed");
        close(fd);
        freeaddrinfo(res);
        return (-1);
    }

    if (bind(fd, res->ai_addr, res->ai_addrlen) < 0)
    {
        Logger::log(Logger::ERROR,
            "bind() failed on " + _listen + " (" + strerror(errno) + ")");
        close(fd);
        freeaddrinfo(res);
        return (-1);
    }

    if (listen(fd, MAX_EVENTS) < 0)
    {
        Logger::log(Logger::ERROR, "listen() failed");
        close(fd);
        freeaddrinfo(res);
        return (-1);
    }

    freeaddrinfo(res);

    poller.addFd(fd, EPOLLIN | EPOLLOUT);

    return (fd);
}

bool    MasterServer::isListenFd(int fd) const
{
    return (listenFdToServers.count(fd) > 0);
}

ServerConfig    *MasterServer::selectServerForRequest(const Request &req, int listenFd)
{
    (void)req;
    std::map<int, ServerConfig *>::const_iterator it = listenFdToServers.find(listenFd);
    if (it == listenFdToServers.end())
        return (NULL);

    ServerConfig    *defaultServer = it->second;
    if (!defaultServer)
        return (NULL);
    return (defaultServer);
}

void    MasterServer::handleAccept(int listenFd)
{
    int flags;
    int clientFd;

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

    poller.addFd(clientFd, EPOLLIN | EPOLLOUT);

    Logger::log(Logger::NEW, "Nova conexão aceita FD " + Utils::toString(clientFd));
}

void    MasterServer::handleRead(int clientFd)
{
    std::map<int, Connection *>::iterator it = connections.find(clientFd);
    if (it == connections.end())
        return ;

    Connection *conn = it->second;

    if (conn->shouldCloseAfterSend())
        return ;

    size_t max_body = 1024 * 1024;
    std::map<int, ServerConfig *>::const_iterator vec =
        listenFdToServers.find(conn->getListenFd());
    if (vec != listenFdToServers.end() && vec->second)
        max_body = vec->second->max_body_size;

    if (conn->is_rejeting)
        return ;
    
    ssize_t n = conn->readFromFd();
    if (n == 0)
    {
        closeConnection(clientFd);
        return ;
    }
    else if (n < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK) 
            closeConnection(clientFd);
        return ;
    }

    if (conn->getInputBuffer().size() > (max_body + 8192))
    {
        Response    res;
        res.status = 413;
        res.body =
            "<html><body>"
            "<h1>413 Payload Too Large</h1>"
            "<a href=\"/\" target=\"_top\">Voltar</a>"
            "</body></html>";

        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        res.headers["Connection"] = "close";

        conn->getOutputBuffer().append(res.toString());
        poller.modifyFd(clientFd, EPOLLOUT);
        
        Logger::log(Logger::NEW, "Status 413 Payload Too Large");
        conn->is_rejeting = true;
        return ;
    }

    int processed = 0;
    bool has_pending_cgi = false;
    while (HttpParser::hasCompleteRequest(conn->getInputBuffer()))
    {
        Request req;
        if (!HttpParser::parseRequest(conn->getInputBuffer(), req, max_body))
            break ;

        if (req.bad_request)
        {
            Response res;
            res.status = 400;
            res.body = "<html><body>"
                      "<h1>400 Bad Request</h1>"
                      "<p>" + req.bad_request_reason + "</p>"
                      "<p>HTTP/1.1 requires a Host header.</p>"
                      "</body></html>";
            res.headers["Content-Type"] = "text/html";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            res.headers["Connection"] = "close";
            
            conn->getOutputBuffer().append(res.toString());
            conn->setCloseAfterSend(true);
            poller.modifyFd(clientFd, EPOLLOUT);
            
            Logger::log(Logger::ERROR, "400 Bad Request: " + req.bad_request_reason);
            return ;
        }

        ServerConfig *sc = selectServerForRequest(req, conn->getListenFd());
        conn->setServer(sc);

        std::string connHdr;
        if (req.headers.count("Connection"))
            connHdr = req.headers.at("Connection");

        if (connHdr == "close")
            conn->setCloseAfterSend(true);
        else if (req.version == "HTTP/1.0" && connHdr != "keep-alive")
            conn->setCloseAfterSend(true);
        else
            conn->setCloseAfterSend(false);

        Logger::log(Logger::INFO,
                    req.method + " " + req.uri + " " + req.version +
                    " recebido na FD " + Utils::toString(clientFd));

        Response res = Router::route(req, *sc, conn);

        if (res.status == 0 && conn->cgi_state)
        {
            // Adicionar stdout do CGI ao epoll para leitura
            poller.addFd(conn->cgi_state->stdout_fd, EPOLLIN | EPOLLOUT);
            cgiFdToClientFd[conn->cgi_state->stdout_fd] = clientFd;

            // Se ainda temos stdin aberto, adicionar para escrita
            if (conn->cgi_state->stdin_fd != -1)
            {
                poller.addFd(conn->cgi_state->stdin_fd, EPOLLIN | EPOLLOUT);
                cgiFdToClientFd[conn->cgi_state->stdin_fd] = clientFd;
            }
            
            has_pending_cgi = true;
            processed++;
            break ;
        }

        // Log do status
        res.status >= 200 && res.status <= 300 ?
            Logger::log(Logger::DEBUG, "Status " + Utils::toString(res.status) +
                " " + Response::reasonPhrase(res.status))
        : res.status >= 400 && res.status <= 500 ?
            Logger::log(Logger::ERROR, "Status " + Utils::toString(res.status) +
                " " + Response::reasonPhrase(res.status))
                        : Logger::log(Logger::WINT, "Status " + Utils::toString(res.status) +
                        " " + Response::reasonPhrase(res.status));

        conn->getOutputBuffer().append(res.toString());
        processed++;
    }

    // CORRIGIDO: Só mudar para escrita se não há CGI pendente
    if (processed > 0 && !has_pending_cgi)
        poller.modifyFd(clientFd, EPOLLOUT);

    // Se há CGI pendente, desativar leitura até CGI terminar
    if (has_pending_cgi)
        poller.modifyFd(clientFd, 0);  // Sem eventos até CGI terminar
}

void    MasterServer::handleWrite(int clientFd)
{
    std::map<int, Connection *>::iterator it = connections.find(clientFd);
    if (it == connections.end())
        return ;

    Connection *conn = it->second;
    Buffer &out = conn->getOutputBuffer();

    if (out.empty())
    {
        conn->waiting_for_write = false;
        poller.modifyFd(clientFd, EPOLLIN);
        return ;
    }

    if (!conn->waiting_for_write)
    {
        conn->waiting_for_write = true;
        conn->write_start_time = time(NULL);
    }

    ssize_t sent = conn->writeToFd(out.data(), out.size());

    if (sent > 0)
        out.consume(sent);
    else if (sent == 0)
    {
        closeConnection(clientFd);
        return ;
    }
    else if (sent < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            closeConnection(clientFd);
        return ;
    }

    if (conn->shouldCloseAfterSend() && out.empty())
        return closeConnection(clientFd);

    if (out.empty())
    {
        conn->waiting_for_write = false;
        poller.modifyFd(clientFd, EPOLLIN);
    }
    else
        poller.modifyFd(clientFd, EPOLLOUT);
}

void    MasterServer::handleCgiRead(int cgiFd)
{
    std::map<int, int>::iterator it = cgiFdToClientFd.find(cgiFd);
    if (it == cgiFdToClientFd.end())
        return ;
    
    int clientFd = it->second;

    std::map<int, Connection *>::iterator conn_it = connections.find(clientFd);
    if (conn_it == connections.end())
        return ;
    
    Connection *conn = conn_it->second;
    if (!conn || !conn->cgi_state)
        return ;
    
    char    buffer[4096];
    ssize_t n = read(cgiFd, buffer, sizeof(buffer));
    
    if (n > 0)
        conn->cgi_state->output.append(buffer, n);
    else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
    {
        // EOF ou erro - CGI terminou de escrever
        poller.removeFd(cgiFd);
        close(cgiFd);
        cgiFdToClientFd.erase(cgiFd);
        conn->cgi_state->stdout_fd = -1;
        
        // Verificar se o processo terminou
        int status;
        pid_t result = waitpid(conn->cgi_state->pid, &status, WNOHANG);
        if (result > 0)
        {
            // Processo terminou
            int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            (void)exit_code;
            finalizeCgi(clientFd);
        }
        else if (result == 0)
            ;
    }
}

void    MasterServer::handleCgiWrite(int cgiFd)
{
    std::map<int, int>::iterator it = cgiFdToClientFd.find(cgiFd);
    if (it == cgiFdToClientFd.end())
        return;
    
    int clientFd = it->second;
    
    std::map<int, Connection *>::iterator conn_it = connections.find(clientFd);
    if (conn_it == connections.end())
        return;
    
    Connection  *conn = conn_it->second;
    if (!conn || !conn->cgi_state || conn->cgi_state->stdin_closed)
        return ;
    
    CgiState    *state = conn->cgi_state;
    
    // Calcular quanto falta escrever
    size_t remaining = state->pending_write.size() - state->write_offset;
    if (remaining == 0)
    {
        // Já escrevemos tudo, fechar stdin
        poller.removeFd(cgiFd);
        close(cgiFd);
        cgiFdToClientFd.erase(cgiFd);
        state->stdin_fd = -1;
        state->stdin_closed = true;
        return ;
    }
    
    // Escrever o que falta
    const char *data = state->pending_write.c_str() + state->write_offset;
    ssize_t written = write(cgiFd, data, remaining);

    if (written > 0)
    {
        state->write_offset += written;

        // Verificar se terminamos
        if (state->write_offset >= state->pending_write.size())
        {
            poller.removeFd(cgiFd);
            close(cgiFd);
            cgiFdToClientFd.erase(cgiFd);
            state->stdin_fd = -1;
            state->stdin_closed = true;
        }
    }
    else if (written < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            // Pipe cheio, tentar depois
            ;
        else
        {
            poller.removeFd(cgiFd);
            close(cgiFd);
            cgiFdToClientFd.erase(cgiFd);
            state->stdin_fd = -1;
            state->stdin_closed = true;
        }
    }
}

void    MasterServer::finalizeCgi(int clientFd)
{
    std::map<int, Connection *>::iterator it = connections.find(clientFd);
    if (it == connections.end())
        return ;
    
    Connection  *conn = it->second;
    if (!conn || !conn->cgi_state)
        return ;
        
    // Remover FDs do epoll se ainda existirem
    if (conn->cgi_state->stdout_fd != -1)
    {
        poller.removeFd(conn->cgi_state->stdout_fd);
        close(conn->cgi_state->stdout_fd);
        cgiFdToClientFd.erase(conn->cgi_state->stdout_fd);
    }
    
    if (conn->cgi_state->stdin_fd != -1)
    {
        poller.removeFd(conn->cgi_state->stdin_fd);
        close(conn->cgi_state->stdin_fd);
        cgiFdToClientFd.erase(conn->cgi_state->stdin_fd);
    }
    
    // Processar output do CGI
    if (!conn->cgi_state->output.empty())
    {
        Response res = CgiHandler::parseCgiOutput(conn->cgi_state->output);
        
        // Log do status
        res.status >= 200 && res.status <= 300 ?
            Logger::log(Logger::DEBUG, "Status " + Utils::toString(res.status) +
                " " + Response::reasonPhrase(res.status))
        : res.status >= 400 && res.status <= 500 ?
            Logger::log(Logger::ERROR, "Status " + Utils::toString(res.status) +
                " " + Response::reasonPhrase(res.status))
                        : Logger::log(Logger::WINT, "Status " + Utils::toString(res.status) +
                        " " + Response::reasonPhrase(res.status));
        
        conn->getOutputBuffer().append(res.toString());
    }
    else
    {
        Response    res;
        res.status = 500;
        res.body = "<h1>500 CGI Empty Output</h1>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        conn->getOutputBuffer().append(res.toString());
    }
    
    // Limpar estado CGI
    delete conn->cgi_state;
    conn->cgi_state = NULL;
    
    // Ativar escrita para enviar resposta
    poller.modifyFd(clientFd, EPOLLOUT);
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
            continue ;
        }
        if (c->waiting_for_write && (now - c->write_start_time) > write_timeout)
        {
            Logger::log(Logger::WARN, "[TIMEOUT] WRITE Timeout FD " + Utils::toString(c->getFd()));
            toClose.push_back(c->getFd());
            continue ;
        }
        if (!c->waiting_for_write && c->getInputBuffer().empty() && idle > keepalive_timeout)
        {
            Logger::log(Logger::WARN, "[TIMEOUT] Keep-Alive Timeout FD " + Utils::toString(c->getFd()));
            toClose.push_back(c->getFd());
            continue ;
        }
    }

    for (size_t i = 0; i < toClose.size(); ++i)
        closeConnection(toClose[i]);
}

void    MasterServer::checkCgiTimeouts()
{
    time_t now = time(NULL);
    std::vector<int> to_kill;
    
    for (std::map<int, Connection *>::iterator it = connections.begin();
         it != connections.end(); ++it)
    {
        Connection  *conn = it->second;
        if (!conn->cgi_state)
            continue ;
        
        ServerConfig    *sc = conn->getServer();
        if (!sc || sc->cgi_timeout <= 0)
            continue ;
        
        if ((now - conn->cgi_state->start_time) > sc->cgi_timeout)
        {
            Logger::log(Logger::WARN, "CGI timeout na FD " + Utils::toString(it->first) +
                       " (PID " + Utils::toString(conn->cgi_state->pid) + ")");
            to_kill.push_back(it->first);
        }
    }
    
    for (size_t i = 0; i < to_kill.size(); ++i)
    {
        Connection *conn = connections[to_kill[i]];
        if (conn->cgi_state)
        {
            if (conn->cgi_state->stdin_fd != -1)
            {
                poller.removeFd(conn->cgi_state->stdin_fd);
                close(conn->cgi_state->stdin_fd);
                cgiFdToClientFd.erase(conn->cgi_state->stdin_fd);
            }
            if (conn->cgi_state->stdout_fd != -1)
            {
                poller.removeFd(conn->cgi_state->stdout_fd);
                close(conn->cgi_state->stdout_fd);
                cgiFdToClientFd.erase(conn->cgi_state->stdout_fd);
            }
            
            kill(conn->cgi_state->pid, SIGKILL);
            waitpid(conn->cgi_state->pid, NULL, 0);
            
            Response    res;
            res.status = 504;
            res.body = "<h1>504 CGI Timeout</h1>";
            res.headers["Content-Type"] = "text/html";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            conn->getOutputBuffer().append(res.toString());
            
            delete conn->cgi_state;
            conn->cgi_state = NULL;
            
            poller.modifyFd(to_kill[i], EPOLLOUT);
        }
    }
}

void    MasterServer::run()
{
    Logger::log(Logger::INFO, "MasterServer iniciado...");
    while (g_running)
    {
        std::vector<PollEvent> events = poller.waitEvents(100);
        
        if (!g_running)
            break ;

        for (size_t i = 0; i < events.size(); ++i)
        {
            int fd = events[i].fd;
            
            if (events[i].isError())
            {
                if (cgiFdToClientFd.count(fd))
                {
                    int clientFd = cgiFdToClientFd[fd];
                    finalizeCgi(clientFd);
                }
                else
                    closeConnection(fd);
                continue ;
            }
            
            if (isListenFd(fd))
            {
                handleAccept(fd);
                continue ;
            }
            
            if (cgiFdToClientFd.count(fd))
            {
                if (events[i].isReadable())
                    handleCgiRead(fd);
                if (events[i].isWritable())
                    handleCgiWrite(fd);
                continue ;
            }
            
            if (events[i].isReadable())
                handleRead(fd);
            if (events[i].isWritable())
                handleWrite(fd);
        }
        
        checkTimeouts();
        checkCgiTimeouts();
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