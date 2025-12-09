#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP


#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include <map>
#include <string>
#include <ctime>

struct SessionData
{
    int visits;
    time_t lastSeen;
};

class SessionManager
{
private:
    std::map<std::string, SessionData> sessions;
    int timeout; // segundos

public:
    SessionManager(int timeoutSeconds);

    std::string createSession();
    bool hasSession(const std::string &sid) const;
    SessionData &getSession(const std::string &sid);
    void updateSession(const std::string &sid);

    void cleanup(); // remove sessões expiradas
};

extern SessionManager g_sessions;

#endif