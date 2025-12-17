#include "SessionManager.hpp"
#include "../http/Router.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include <sstream>
#include <cstdlib>

SessionManager  g_sessions(60 * 5);

static std::string  generateRandomId()
{
    std::stringstream ss;
    ss << std::hex << rand() << rand();
    return ss.str();
}

SessionManager::SessionManager(int timeoutSeconds)
    : timeout(timeoutSeconds)
{
}

std::string SessionManager::createSession()
{
    SessionData d;

    d.csrf = generateRandomId();
    d.visits = 0;
    d.logged = false;
    d.lastSeen = std::time(NULL);

    sessions[d.csrf] = d;

    return (d.csrf);
}

bool    SessionManager::hasSession(const std::string &sid) const
{
    return (sessions.find(sid) != sessions.end());
}

SessionData &SessionManager::getSession(const std::string &sid)
{
    return  sessions[sid];
}

void    SessionManager::updateSession(const std::string &sid)
{
    std::map<std::string, SessionData>::iterator it = sessions.find(sid);
    if (it != sessions.end())
    {
        it->second.visits++;
        it->second.lastSeen = std::time(NULL);
    }
}

void    SessionManager::cleanup()
{
    std::map<std::string, SessionData>::iterator it = sessions.begin();

    time_t now = std::time(NULL);

    while (it != sessions.end())
    {
        if ((now - it->second.lastSeen) > timeout)
            sessions.erase(it++);
        else
            ++it;
    }
}

void    SessionManager::saveToFile(const std::string &path)
{
    std::ofstream f(path.c_str());
    if (!f.is_open()) return;

    std::map<std::string, SessionData>::iterator it;
    for (it = sessions.begin(); it != sessions.end(); ++it)
    {
        f << it->first << " "
          << it->second.visits << " "
          << it->second.lastSeen << " "
          << it->second.csrf << " "
          << it->second.logged << "\n";
    }
}

void SessionManager::loadFromFile(const std::string &path)
{
    std::ifstream f(path.c_str());
    if (!f.is_open()) return;

    sessions.clear();

    while (!f.eof())
    {
        std::string sid;
        SessionData d;
        int logged;

        f >> sid >> d.visits >> d.lastSeen >> d.csrf >> logged;
        d.logged = logged;
        sessions[sid] = d;
    }
}

bool    Router::handleSession(const Request &req, Response &res)
{
    if (!(req.method == "GET" && req.uri == "/session"))
        return false;

    std::string sid = "";

    if (req.headers.count("Cookie"))
    {
        std::string ck = req.headers.at("Cookie");
        size_t pos = ck.find("session_id=");
        if (pos != std::string::npos)
        {
            sid = ck.substr(pos + 11);
            size_t end = sid.find(';');
            if (end != std::string::npos)
                sid = sid.substr(0, end);
        }
    }

    if (sid.empty() || !g_sessions.hasSession(sid))
    {
        res.status = 403;
        res.body = "<h1 class=\"error\">403</h1><p class=\"statusR\">Sessão inexistente<p>"
                "<button onclick=\"login()\" class=\"logintbtn\">Login</button><br><br>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return true;
    }

    SessionData &data = g_sessions.getSession(sid);
    g_sessions.updateSession(sid);

    res.status = 200;
    res.body = "<p style=\"width: 250px; margin-top: 10px; line-height: 1.4;margin-bottom: 10px;\">Observe as informação sobre as sessão de usuário</p>"
                "<p class=\"statusG\">Sessão ativa!</p>"
               "<p>Visitas: " + Utils::toString(data.visits)
               + "</p><button onclick=\"updatepPage()\" class=\"updatetbtn\">Atualizar</button><br><br>"
               "</p><button onclick=\"logout()\" class=\"logoutbtn\">Logout</button><br><br>";
    res.headers["Content-Type"] = "text/html";
    res.headers["Content-Length"] = Utils::toString(res.body.size());
    return true;
}

bool    Router::handleSessionGeneric(const Request &req, Response &res)
{
    if (!(req.method == "GET"))
        return false;

    std::string sid = "";

    if (req.headers.count("Cookie"))
    {
        std::string ck = req.headers.at("Cookie");
        size_t pos = ck.find("session_id=");
        if (pos != std::string::npos)
        {
            sid = ck.substr(pos + 11);
            size_t end = sid.find(';');
            if (end != std::string::npos)
                sid = sid.substr(0, end);
        }
    }

    if ((sid.empty() || !g_sessions.hasSession(sid)) && req.uri != "/session")
    {
        std::cout << "Sessão inexistente na rota genérica." << std::endl;
        res.status = 403;
        res.body = "<h1 class=\"error\">403</h1><p class=\"statusR\">Sessão inexistente<p>"
                "<button onclick=\"login()\" class=\"logintbtn\">Login</button><br><br>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return true;
    }
    std::cout << "Sessão encontrada na rota genérica." << std::endl;

    SessionData &data = g_sessions.getSession(sid);
    (void) data;
    g_sessions.updateSession(sid);
    return false;
}

bool    Router::handleCsrf(const Request &req, Response &res)
{
    if (!(req.method == "GET" && req.uri == "/csrf"))
        return false;

    std::string token = g_sessions.createSession();

    res.status = 200;
    res.body = "{\"csrf\": \"" + token + "\"}";
    res.headers["Content-Type"] = "application/json";
    res.headers["Content-Length"] = Utils::toString(res.body.size());
    res.headers["Set-Cookie"] = "csrf_token=" + token + "; Path=/; HttpOnly";

    return true;
}

bool    Router::handleLogin(const Request &req, Response &res)
{
    if (!(req.method == "POST" && req.uri == "/login"))
        return false;

    // token enviado no body
    std::string sent_token = req.body;

    // token do cookie
    std::string csrf_cookie = "";
    if (req.headers.count("Cookie"))
    {
        std::string ck = req.headers.at("Cookie");
        size_t pos = ck.find("csrf_token=");
        if (pos != std::string::npos)
        {
            csrf_cookie = ck.substr(pos + 11);
            size_t end = csrf_cookie.find(';');
            if (end != std::string::npos)
                csrf_cookie = csrf_cookie.substr(0, end);
        }
    }

    std::cout << "Sent token: " << sent_token << ", Cookie token: " << csrf_cookie << std::endl;

    if (sent_token.find(csrf_cookie) == std::string::npos)
    {
        res.status = 403;
        res.body = "<h1>403 - CSRF inválido</h1>";
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return true;
    }

    std::string sid = g_sessions.createSession();

    res.status = 200;
    if (fileExists("examples/www/site1/sessions.html"))
        res.body = readFile("examples/www/site1/sessions.html");
    else
        res.body = "<h1>Login OK!</h1><a href=\"/sessions.html\">Prosseguir</a>";
    res.headers["Set-Cookie"] = "session_id=" + sid + "; Path=/; HttpOnly";
    res.headers["Content-Type"] = "text/html";
    res.headers["Content-Length"] = Utils::toString(res.body.size());
    return true;
}

bool    Router::handleLogout(const Request &req, Response &res)
{
    if (!(req.method == "POST" && req.uri == "/logout"))
        return false;

    std::string sid = "";
    if (req.headers.count("Cookie"))
    {
        std::string ck = req.headers.at("Cookie");
        size_t pos = ck.find("session_id=");
        if (pos != std::string::npos)
            sid = ck.substr(pos + 11);
    }

    res.status = 200;
    res.body = "Logout OK!";
    res.headers["Set-Cookie"] = "session_id=; Path=/; Max-Age=0; HttpOnly";
    res.headers["Content-Type"] = "text/plain";
    res.headers["Content-Length"] = Utils::toString(res.body.size());

    return true;
}