#include "SessionManager.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include <sstream>
#include <cstdlib>

SessionManager g_sessions(60 * 10); // 10 minutos por exemplo

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
    std::string sid = generateRandomId();

    SessionData d;
    d.visits = 1;
    d.lastSeen = std::time(NULL);

    sessions[sid] = d;

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
    std::map<std::string, SessionData>::iterator it = sessions.find(sid);
    if (it != sessions.end())
    {
        it->second.visits++;
        it->second.lastSeen = std::time(NULL);
    }
}

void SessionManager::cleanup()
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