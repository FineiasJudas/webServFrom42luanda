#include <sstream>
#include <dirent.h>
#include "Router.hpp"
#include "../utils/Utils.hpp"
#include "../utils/Logger.hpp"
#include "../config/Config.hpp"
#include "../http/Response.hpp"
#include "../cgi/CgiHandler.hpp"
#include "../session/SessionManager.hpp"

static std::string getExtension(const std::string &path)
{
    size_t pos = path.rfind('.');

    if (pos == std::string::npos)
        return "";
    return path.substr(pos);
}

bool matchLocation(const std::string &uri, const std::string &locPath)
{
    if (uri.find(locPath) != 0)
        return (false);

    // location termina com /
    if (locPath[locPath.size() - 1] == '/')
        return (true);

    if (uri.size() == locPath.size())
        return (true);

    if (uri[locPath.size()] == '/')
        return (true);

    return (false);
}

const LocationConfig &findBestLocation(const std::string &uri,
                                       const ServerConfig &config)
{
    const LocationConfig *best = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < config.locations.size(); i++)
    {
        const LocationConfig &loc = config.locations[i];

        if (loc.path.empty() || loc.path[0] != '/')
            continue;

        if (matchLocation(uri, loc.path))
        {
            if (loc.path.size() > bestLen)
            {
                best = &loc;
                bestLen = loc.path.size();
            }
        }
    }

    if (!best)
        return config.locations[0]; // "/"

    return (*best);
}

void parseUri(Request &req)
{
    size_t pos = req.uri.find('?');

    if (pos == std::string::npos)
    {
        req.path = req.uri;
        return;
    }

    req.path = req.uri.substr(0, pos);

    std::string qs = req.uri.substr(pos + 1);
    std::stringstream ss(qs);
    std::string pair;

    while (std::getline(ss, pair, '&'))
    {
        size_t eq = pair.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = pair.substr(0, eq);
        std::string val = pair.substr(eq + 1);

        req.query[key] = val;
    }
}

//   LISTAR ARQUIVOS EM /uploads-list
Response handleUploadsList(const Request &req, const ServerConfig &config)
{
    (void)req;

    Response res;
    size_t i = 0;
    for (; i < config.locations.size(); i++)
    {
        if (config.locations[i].path == "/uploads-list")
        {
            if (config.locations[i].root.empty())
            {
                res.status = 500;
                res.body = "{\"error\": \"uploads-list root not configured\"}";
                res.headers["Content-Type"] = "application/json";
                res.headers["Content-Length"] = Utils::toString(res.body.size());
                return res;
            }
            break;
        }
    }

    std::string folder = config.locations[i].root;

    DIR *dir = opendir(folder.c_str());
    if (!dir)
    {
        res.status = 500;
        res.body = "{\"error\": \"cannot open uploads folder\"}";
        res.headers["Content-Type"] = "application/json";
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    std::stringstream json;
    json << "{ \"files\": [";

    bool first = true;
    struct dirent *entry;

    while ((entry = readdir(dir)))
    {
        if (entry->d_name[0] == '.')
            continue;

        if (!first)
            json << ",";
        first = false;

        json << "\"" << entry->d_name << "\"";
    }

    json << "] }";
    closedir(dir);

    res.status = 200;
    res.body = json.str();
    res.headers["Content-Type"] = "application/json";
    res.headers["Content-Length"] = Utils::toString(res.body.size());
    return res;
}

//   DELETE EM /delete-file?name=
Response handleDeleteFile(const Request &req, const ServerConfig &config)
{
    Response r;

    if (req.query.find("name") == req.query.end())
    {

        r.status = 400;
        r.body = "{\"error\": \"missing ?name=\"}";
        r.headers["Content-Type"] = "application/json";
        r.headers["Content-Length"] = Utils::toString(r.body.size());
        return r;
    }

    size_t p;
    std::string filename = req.query.at("name");

    while ((p = filename.find("%20")) != std::string::npos)
    {
        filename.erase(p + 1, 2);
        filename[p] = ' ';
    }

    size_t i = 0;
    for (; i < config.locations.size(); i++)
    {
        if (config.locations[i].path == "/uploads-list")
        {
            if (config.locations[i].root.empty())
            {
                r.status = 500;
                r.body = "{\"error\": \"upload_dir root not configured\"}";
                r.headers["Content-Type"] = "application/json";
                r.headers["Content-Length"] = Utils::toString(r.body.size());
                return r;
            }
            break;
        }
    }

    std::string fullpath = config.locations[i].root + "/" + filename;

    if (fileExists(fullpath))
        std::remove(fullpath.c_str());
    else
    {
        r.status = 500;
        r.body = "{\"error\": \"failed to delete\"}";
        r.headers["Content-Type"] = "application/json";
        r.headers["Content-Length"] = Utils::toString(r.body.size());
        return r;
    }

    r.status = 200;
    r.body = "{\"status\": \"deleted\"}";
    r.headers["Content-Type"] = "application/json";
    r.headers["Content-Length"] = Utils::toString(r.body.size());
    return r;
}

static std::string makeRealPath(const std::string &uri,
                                const LocationConfig &loc,
                                const ServerConfig &srv)
{
    std::string root = loc.root.empty() ? srv.root : loc.root;
    std::string suffix;

    if (uri.size() >= loc.path.size())
        suffix = uri.substr(loc.path.size());

    if (!root.empty() && root[root.size() - 1] == '/' &&
        !suffix.empty() && suffix[0] == '/')
        return root + suffix.substr(1);

    if (!root.empty() && root[root.size() - 1] != '/' &&
        !suffix.empty() && suffix[0] != '/')
        return root + "/" + suffix;

    return root + suffix;
}

Response    Router::route(const Request &req, const ServerConfig &config, Connection *conn)
{
    Response r;
    Request rq = req;


    /* 1 Rotas internas */
    if (handleCsrf(rq, r))
        return r;
    if (handleLogin(rq, r))
        return r;
    if (handleLogout(rq, r))
        return r;
    if (handleSession(rq, r))
        return r;

    /* 2 Encontrar location */
    const LocationConfig &loc = findBestLocation(rq.uri, config);

    /* 3 Redirect */
    if (loc.redirect_code)
    {
        r.status = loc.redirect_code;
        r.headers["Location"] = loc.redirect_url;
        r.body = "<h1>Redirect</h1>";
        r.headers["Content-Length"] = Utils::toString(r.body.size());
        r.headers["Content-Type"] = "text/html";
        return r;
    }

    /* 4 Segurança */
    if (rq.uri.find("..") != std::string::npos)
        return forbiddenPageResponse(config);

    /* 5 Resolver FS path */
    std::string fsPath = makeRealPath(rq.path, loc, config);

    /* 7 Métodos HTTP */
    std::string busca = req.method;

    bool metodo_req_existe = std::find(loc.methods.begin(), loc.methods.end(), busca) != loc.methods.end();

    if (!metodo_req_existe)
        return notAloweMethodResponse(config);

    // daqui pra frente o método é permitido
    std::string ext = getExtension(rq.path);

    // 1 Se não for GET → só CGI
    if (req.method != "GET" && ext != "")
    {
        for (size_t i = 0; i < loc.cgi.size(); i++)
        {
            if (loc.cgi[i].extension == ext)
                return CgiHandler::handleCgiRequest(rq, config, loc, loc.cgi[i], conn);
        }

        return notAloweMethodResponse(config);
    }

    // 2 GET
    for (size_t i = 0; i < loc.cgi.size(); i++)
    {
        if (loc.cgi[i].extension == ext)
            return CgiHandler::handleCgiRequest(rq, config, loc, loc.cgi[i], conn);
    }

    // 3 GET estático
    return methodGet(config, loc, fsPath, rq.uri);

}