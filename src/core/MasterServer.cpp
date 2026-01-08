#include "MasterServer.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"
#include "../http/HttpParser.hpp"
#include "../http/Router.hpp"
#include "../../includes/Headers.hpp"
#include "../exceptions/WebServException.hpp"
#include "./../utils/keywords.hpp"
#include "../session/SessionManager.hpp"
#include "../cgi/CgiHandler.hpp"

volatile sig_atomic_t g_running = 1;

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

void MasterServer::createListenSockets(const std::vector<ServerConfig> &servers)
{
    int fd;
    int port;

    for (size_t i = 0; i < servers.size(); ++i)
    {
        const ServerConfig &sc = servers[i];

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

    struct sockaddr_in addr;
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

void MasterServer::handleAccept(int listenFd)
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
/*
void MasterServer::handleRead(int clientFd)
{
    std::map<int, Connection *>::iterator it = connections.find(clientFd);
    if (it == connections.end())
        return;

    size_t max_body = 1024 * 1024; // 1 MB por defeito. Este valor deve estar no header como macro
    Connection *conn = it->second;
    std::map<int, ServerConfig *>::const_iterator vec = listenFdToServers.find(conn->getListenFd());
    if (vec != listenFdToServers.end() && vec->second)
        max_body = vec->second->max_body_size;

    // Ler UMA vez (epoll LT)
    ssize_t n = conn->readFromFd();
    if (conn->getInputBuffer().size() > max_body)
    {
        Response res;
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

        // muda para escrita
        poller.modifyFd(clientFd, EPOLLOUT | EPOLLET);

        // marca para fechar depois
        conn->setCloseAfterSend(true);

        Logger::log(Logger::NEW,
                    "Status 413 Payload Too Large");

        return;
    }

    if (n <= 0)
    {closeAfterSend
        closeConnection(clientFd);
        return;
    }// nada para ler agora idiota e nem precisas diferenciar erros

    // Processar requisições completas
    int processed = 0;
    while (HttpParser::hasCompleteRequest(conn->getInputBuffer()))
    {
        Request req;

        bool ok = HttpParser::parseRequest(conn->getInputBuffer(), req, max_body);
        if (!ok)
            break;
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

        res.status >= 200 && res.status <= 300   ? Logger::log(Logger::DEBUG, "Status " + Utils::toString(res.status) +
                                                                                  " " + Response::reasonPhrase(res.status))
        : res.status >= 400 && res.status <= 500 ? Logger::log(Logger::ERROR, "Status " + Utils::toString(res.status) + " " + Response::reasonPhrase(res.status))
                                                 : Logger::log(Logger::WINT, "Status " + Utils::toString(res.status) + " " + Response::reasonPhrase(res.status));

        conn->getOutputBuffer().append(res.toString());
        conn->getInputBuffer().clear();
        processed++;
    }

    if (processed > 0)
        poller.modifyFd(clientFd, EPOLLOUT);
}
*/

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
    {
        Logger::log(Logger::WARN,
                "Conexao do FD" + Utils::toString(clientFd) + " rejeitada!");
    }
    
    ssize_t n = conn->readFromFd();
    Logger::log(Logger::WINT,
                "Lidos " + Utils::toString(n) + " bytes da FD " +
                Utils::toString(clientFd) + ", buffer de entrada tem " +
                Utils::toString(conn->getInputBuffer().size()) + " bytes");

    if (n == 0)
    {
        closeConnection(clientFd);
        return;
    }
    if (n < 0)
    {
        return ;
    }

    if (conn->getInputBuffer().size() > max_body)
    {
        Response res;
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
        return;
    }

    int processed = 0;
    bool has_pending_cgi = false;  // NOVO

    while (HttpParser::hasCompleteRequest(conn->getInputBuffer()))
    {
        Request req;
        if (!HttpParser::parseRequest(conn->getInputBuffer(), req, max_body))
            break ;

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

        // NOVO: Verificar se é CGI pendente
        if (res.status == 0 && conn->cgi_state)
        {
            Logger::log(Logger::INFO, "CGI pendente detectado, aguardando conclusão...");
            
            // Adicionar stdout do CGI ao epoll para leitura
            poller.addFd(conn->cgi_state->stdout_fd, EPOLLIN);
            cgiFdToClientFd[conn->cgi_state->stdout_fd] = clientFd;  // MAPEAR
            
            // Se ainda temos stdin aberto, adicionar para escrita
            if (conn->cgi_state->stdin_fd != -1)
            {
                poller.addFd(conn->cgi_state->stdin_fd, EPOLLOUT);
                cgiFdToClientFd[conn->cgi_state->stdin_fd] = clientFd;  // MAPEAR
            }
            
            has_pending_cgi = true;  // MARCAR
            processed++;
            break;  // Importante: parar de processar mais requests
        }

        // Log do status
        (res.status >= 200 && res.status <= 300)
            ? Logger::log(Logger::DEBUG,
                          "Status " + Utils::toString(res.status) + " " +
                          Response::reasonPhrase(res.status))
            : Logger::log(Logger::ERROR,
                          "Status " + Utils::toString(res.status) + " " +
                          Response::reasonPhrase(res.status));

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

    if (out.empty()) // nada para escrever
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
        return ;
    else
        return ;

    if (conn->shouldCloseAfterSend() && out.empty())
        return closeConnection(clientFd);

    if (out.empty()) // terminou de escrever
    {
        conn->waiting_for_write = false;
        poller.modifyFd(clientFd, EPOLLIN);
    }
    else
        poller.modifyFd(clientFd, EPOLLOUT); // continuar a escutar escrita
}

// Implementação
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
    {
        conn->cgi_state->output.append(buffer, n);
        Logger::log(Logger::INFO, "CGI: Lidos " + Utils::toString(n) + 
                   " bytes do stdout (total: " + 
                   Utils::toString(conn->cgi_state->output.size()) + ")");
    }
    else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
    {
        Logger::log(Logger::INFO, "CGI: EOF no stdout, aguardando término do processo");
        
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
            Logger::log(Logger::INFO, "CGI: Processo terminou com código " + 
                       Utils::toString(exit_code));
            finalizeCgi(clientFd);
        }
        else if (result == 0)
        {
            Logger::log(Logger::INFO, "CGI: Processo ainda ativo após EOF no stdout");
            // Isso é estranho, mas pode acontecer
            // Dar um tempo e depois finalizar
        }
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
    
    Connection *conn = conn_it->second;
    if (!conn || !conn->cgi_state || conn->cgi_state->stdin_closed)
        return;
    
    CgiState *state = conn->cgi_state;
    
    // Calcular quanto falta escrever
    size_t remaining = state->pending_write.size() - state->write_offset;
    if (remaining == 0)
    {
        // Já escrevemos tudo, fechar stdin
        Logger::log(Logger::INFO, "CGI: Tudo escrito, fechando stdin");
        poller.removeFd(cgiFd);
        close(cgiFd);
        cgiFdToClientFd.erase(cgiFd);
        state->stdin_fd = -1;
        state->stdin_closed = true;
        return;
    }
    
    // Escrever o que falta
    const char *data = state->pending_write.c_str() + state->write_offset;
    ssize_t written = write(cgiFd, data, remaining);
    
    if (written > 0)
    {
        state->write_offset += written;
        Logger::log(Logger::INFO, "CGI: Escritos mais " + Utils::toString(written) + 
                   " bytes, total: " + Utils::toString(state->write_offset) + 
                   " de " + Utils::toString(state->pending_write.size()));
        
        // Verificar se terminamos
        if (state->write_offset >= state->pending_write.size())
        {
            Logger::log(Logger::INFO, "CGI: Escrita completa, fechando stdin");
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
            Logger::log(Logger::WARN, "CGI: Pipe stdin cheio, aguardando...");
        else
        {
            // Erro fatal
            Logger::log(Logger::ERROR, "CGI: Erro ao escrever stdin: " + 
                       std::string(strerror(errno)));
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
        return;
    
    Connection *conn = it->second;
    if (!conn || !conn->cgi_state)
        return;
    
    Logger::log(Logger::INFO, "Finalizando CGI para FD " + Utils::toString(clientFd));
    
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
        Logger::log(Logger::INFO, "CGI: Processando " + 
                    Utils::toString(conn->cgi_state->output.size()) + " bytes de output");
        
        Response res = CgiHandler::parseCgiOutput(conn->cgi_state->output);
        
        // Log do status
        (res.status >= 200 && res.status <= 300)
            ? Logger::log(Logger::DEBUG,
                          "Status " + Utils::toString(res.status) + " " +
                          Response::reasonPhrase(res.status))
            : Logger::log(Logger::ERROR,
                          "Status " + Utils::toString(res.status) + " " +
                          Response::reasonPhrase(res.status));
        
        conn->getOutputBuffer().append(res.toString());
    }
    else
    {
        Logger::log(Logger::WARN, "CGI: Output vazio!");
        Response res;
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

void    MasterServer::checkCgiTimeouts()
{
    time_t now = time(NULL);
    std::vector<int> to_kill;
    
    for (std::map<int, Connection *>::iterator it = connections.begin();
         it != connections.end(); ++it)
    {
        Connection *conn = it->second;
        if (!conn->cgi_state)
            continue ;
        
        ServerConfig *sc = conn->getServer();
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
            // Limpar FDs do epoll
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
            
            // Matar processo
            kill(conn->cgi_state->pid, SIGKILL);
            waitpid(conn->cgi_state->pid, NULL, 0);
            
            // Enviar resposta de timeout
            Response res;
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
        std::vector<PollEvent> events = poller.waitEvents(1000);
        
        if (!g_running)
            break;
        
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
            
            // Verificar se é FD de CGI
            if (cgiFdToClientFd.count(fd))
            {
                if (events[i].isReadable())
                    handleCgiRead(fd);
                if (events[i].isWritable())
                    handleCgiWrite(fd);
                continue ;
            }
            
            // FD de cliente normal
            if (events[i].isReadable())
                handleRead(fd);
            if (events[i].isWritable())
                handleWrite(fd);
        }
        
        checkTimeouts();
        checkCgiTimeouts();  // NOVO
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