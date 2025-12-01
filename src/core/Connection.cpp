#include "Connection.hpp"
#include <unistd.h>
#include "../http/Router.hpp"
#include "../http/HttpParser.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"

Connection::Connection(int fd)
    : fd(fd),
      listenFd(-1),
      server(NULL),
      closeAfterSend(false),
      last_activity_time(time(NULL)),
      waiting_for_write(false),
      write_start_time(0)
{
}

Connection::~Connection()
{
    ::close(fd);
}

ssize_t Connection::readFromFd()
{
    char buffer[4096];
    ssize_t bytes = ::read(fd, buffer, sizeof(buffer));
    if (bytes > 0)
    {
        input_buffer.append(buffer, bytes);
        updateActivity();
    }
    return bytes;
}

ssize_t Connection::writeToFd(const char* data, size_t size)
{
    ssize_t bytes = ::write(fd, data, size);
    if (bytes > 0)
        updateActivity();
    return bytes;
}

int     Connection::getFd() const { return fd; }

void    Connection::setListenFd(int lf) { listenFd = lf; }

int     Connection::getListenFd() const { return listenFd; }

void    Connection::setServer(ServerConfig* s) { server = s; }

ServerConfig*   Connection::getServer() const { return server; }

Buffer  &Connection::getInputBuffer() { return input_buffer; }

Buffer  &Connection::getOutputBuffer() { return output_buffer; }

void    Connection::setCloseAfterSend(bool v) { closeAfterSend = v; }

bool    Connection::shouldCloseAfterSend() const { return closeAfterSend; }

void    Connection::updateActivity() { last_activity_time = time(NULL); }

bool    Connection::handleRequest()
{
    int     clientFd = this->fd;
    int     iteration = 0;

    Logger::log(Logger::INFO, "📨 Dados acumulados na conexão FD "
        + Utils::toString(clientFd) + ": " + Utils::toString(this->input_buffer.size()) + " bytes");

    while (HttpParser::hasCompleteRequest(this->input_buffer))
    {
        Request req;

        // --- PARSER ---
        if (!HttpParser::parseRequest(this->input_buffer, req, 
            server->max_body_size))
        {
            Response    res;
            res.status = 400;
            res.body = "<h1>400 Bad Request</h1>""<a href=\"/\">Voltar</a>";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            res.headers["Content-Type"] = "text/html";

            this->input_buffer.append(res.toString());
            Logger::log(Logger::ERROR, "Status 400 Bad Request na FD "
                + Utils::toString(clientFd));

            return true; // há resposta para enviar
        }

        // --- 413 BODY TOO LARGE ---
        if (req.too_large_body)
        {
            Response    res;
            res.status = 413;
            res.body = "<h1>413 Payload Too Large</h1>""<a href=\"/\">Voltar</a>";
            res.headers["Content-Length"] = Utils::toString(res.body.size());
            res.headers["Content-Type"] = "text/html";

            this->input_buffer.append(res.toString());
            Logger::log(Logger::ERROR,"Status 413 Payload Too Large na FD "
                + Utils::toString(clientFd));

            return true;
        }

        // --- CONNECTION KEEP-ALIVE / CLOSE ---
        if (req.headers["Connection"] == "close")
            this->setCloseAfterSend(true);
        else if (req.version == "HTTP/1.0" &&
                 req.headers["Connection"] != "keep-alive")
            this->setCloseAfterSend(true);
        else
            this->setCloseAfterSend(false);

        // --- ROUTING ---
        Logger::log(Logger::INFO, req.method + " " + req.uri + " " + req.version +
            " recebido na FD " + Utils::toString(clientFd));

        Response res = Router::route(req, *server);

        Logger::log((res.status >= 500) ? Logger::WARN : (res.status >= 400)
            ? Logger::ERROR : Logger::DEBUG,"Status " + Utils::toString(res.status) 
                + " " + Response::reasonPhrase(res.status));

        // --- ADD TO OUTPUT BUFFER ---
        this->input_buffer.append(res.toString());

        iteration++;
    }

    if (iteration > 0)
        Logger::log(Logger::INFO,
            "✅ " + Utils::toString(iteration)
            + " request(s) processado(s) na FD "
            + Utils::toString(clientFd));

    return this->input_buffer.size() > 0;
}
