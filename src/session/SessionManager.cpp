#include "SessionManager.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include <sstream>
#include <cstdlib>

static SessionManager   g_sessions(300); // 5 minutos

SessionManager::SessionManager(int timeoutSeconds)
    : timeout(timeoutSeconds)
{
}

std::string randomId()
{
    std::stringstream ss;
    for (int i = 0; i < 16; ++i)
        ss << std::hex << (rand() % 16);
    return ss.str();
}

std::string SessionManager::createSession()
{
    std::string sid = randomId();
    SessionData data;

    data.visits = 1;
    data.lastSeen = time(NULL);

    sessions[sid] = data;
    return sid;
}

bool SessionManager::hasSession(const std::string &sid) const
{
    return sessions.find(sid) != sessions.end();
}

SessionData &SessionManager::getSession(const std::string &sid)
{
    return sessions[sid];
}

void SessionManager::updateSession(const std::string &sid)
{
    sessions[sid].visits++;
    sessions[sid].lastSeen = time(NULL);
}

void    SessionManager::cleanup()
{
    time_t now = time(NULL);
    std::map<std::string, SessionData>::iterator it = sessions.begin();

    while (it != sessions.end())
    {
        if (now - it->second.lastSeen > timeout)
        {
            std::map<std::string, SessionData>::iterator eraseIt = it;
            ++it;
            sessions.erase(eraseIt);
        }
        else
            ++it;
    }
}

bool    SessionManager::handleSession(Request &req, Response &res)
{
    // Só trata /session
    if (!(req.method == "GET" && req.uri == "/session"))
        return false;

    std::string sid;

    // Buscar session_id do Cookie
    if (req.headers.count("Cookie"))
    {
        std::string ck = req.headers["Cookie"];
        size_t pos = ck.find("session_id=");
        if (pos != std::string::npos)
        {
            sid = ck.substr(pos + 11);
            size_t semi = sid.find(';');
            if (semi != std::string::npos)
                sid = sid.substr(0, semi);
        }
    }

    // Sessão EXISTE
    if (!sid.empty() && g_sessions.hasSession(sid))
    {
        SessionData &data = g_sessions.getSession(sid);
        g_sessions.updateSession(sid);

        res.status = 200;
        res.headers["Content-Type"] = "text/html";

        res.body =
            "<h1>Olá novamente!</h1>"
            "<p>Visitas nesta sessão: " +
            Utils::toString(data.visits) +
            "</p>";

        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return true; // <-- ROTA RESOLVIDA
    }

    // Criar nova sessão
    sid = g_sessions.createSession();

    res.status = 200;
    res.headers["Content-Type"] = "text/html";
    res.headers["Set-Cookie"] = "session_id=" + sid + "; Path=/; HttpOnly";

    res.body =
        "<h1>Bem-vindo! Sessão criada.</h1>"
        "<p>Visitas: 1</p>";

    res.headers["Content-Length"] = Utils::toString(res.body.size());
    return true; // <-- ROTA RESOLVIDA
}