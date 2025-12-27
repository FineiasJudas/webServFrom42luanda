#include <sstream>
#include <dirent.h>
#include "Router.hpp"
#include "../utils/Utils.hpp"
#include "../utils/Logger.hpp"
#include "../config/Config.hpp"
#include "../http/Response.hpp"
#include "../cgi/CgiHandler.hpp"
#include "../session/SessionManager.hpp"

static std::string  getExtension(const std::string &path)
{
    size_t pos = path.rfind('.');

    if (pos == std::string::npos)
        return "";
    return path.substr(pos);
}

bool    matchLocation(const std::string &uri, const std::string &locPath)
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


void    parseUri(Request &req)
{
    size_t pos = req.uri.find('?');

    if (pos == std::string::npos)
    {
        req.path = req.uri;
        return;
    }

    req.path = req.uri.substr(0, pos);

    std::string qs = req.uri.substr(pos + 1);
    std::stringstream   ss(qs);
    std::string pair;

    while (std::getline(ss, pair, '&'))
    {
        size_t  eq = pair.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = pair.substr(0, eq);
        std::string val = pair.substr(eq + 1);

        req.query[key] = val;
    }
}

//   LISTAR ARQUIVOS EM /uploads-list
Response    handleUploadsList(const Request &req, const ServerConfig &config)
{
    (void)req;

    Response    res;
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
            break ;
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

        if (!first) json << ",";
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
Response    handleDeleteFile(const Request &req, const ServerConfig &config)
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
            break ;
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


Response    Router::route(const Request &req, const ServerConfig &config)
{
    Response r;
    Request rq = req;

    /* 1 Rotas internas */
    if (handleCsrf(rq, r)) return r;
    if (handleLogin(rq, r)) return r;
    if (handleLogout(rq, r)) return r;
    if (handleSession(rq, r)) return r;

    if (rq.method == "GET" && rq.uri == "/uploads-list")
        return handleUploadsList(rq, config);

    if (rq.method == "DELETE" && rq.uri.rfind("/delete-file", 0) == 0)
    {
        Request tmp = rq; parseUri(tmp);
        return handleDeleteFile(tmp, config);
    }

    /* 2 Encontrar location */
    const LocationConfig &loc = findBestLocation(rq.uri, config);
    Logger::log(Logger::INFO, "Location escolhida: " + loc.path);

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
    Logger::log(Logger::INFO, "FS PATH: " + fsPath);

    /* 6 CGI
        Estou a trabalhar aqui
    */
    std::string ext = getExtension(rq.path);
    Logger::log(Logger::INFO, "::::::::::::::::CGI EXT: " + ext);
    for (size_t i = 0; i < loc.cgi.size(); i++)
    {
        Logger::log(Logger::INFO, "Comparando com: " + loc.cgi[i].extension);
        if (loc.cgi[i].extension == ext){
            Logger::log(Logger::INFO, "Rota CGI para extensão: " + ext);
            return CgiHandler::handleCgiRequest(rq, config, loc, loc.cgi[i]);
        }
    }

    /* 7 Métodos HTTP */
    if (rq.method == "GET")
        return methodGet(config, loc, fsPath, rq.uri);

    if (rq.method == "POST")
    {
        if (!loc.upload_dir.empty())
            return methodPostMultipart(rq, loc.upload_dir, loc, config);
        return methodPost(rq, config, fsPath, loc);
    }

    if (rq.method == "DELETE")
        return methodDelete(fsPath, config, loc);

    return notAloweMethodResponse(config);
}


/*Response    Router::route(const Request &req, const ServerConfig &config)
{
    // 0) ENDPOINTS ESPECIAIS SEM PASSAR POR LOCATION
    // (Eles sempre devem ser processados *antes* do findBestLocation)

    
    Response r;
    Request rq = req;

    // Rotas especiais (antes de location)
    if (handleCsrf(req, r)) return r;
    if (handleLogin(req, r)) return r;
    if (handleLogout(req, r)) return r;
    if (handleSession(req, r)) return r;

    if (rq.method == "GET" && rq.uri == "/uploads-list")
        return handleUploadsList(rq, config);

    if (rq.method == "DELETE" && rq.uri.rfind("/delete-file", 0) == 0)
    {
        Request tmp = rq;
        parseUri(tmp);
        return handleDeleteFile(tmp, config);
    }

    // 1) Encontrar location correto
    const LocationConfig &loc = findBestLocation(rq.uri, config);

    std::cout << std::endl;
    Logger::log(Logger::INFO, "Rota encontrada: " + loc.path);
    std::cout << std::endl;

    // 1.5) Redirecionamento
    if (loc.redirect_code != 0)
    {
        Response    r;

        r.status = loc.redirect_code;
        r.headers["Location"] = loc.redirect_url;
        r.body = "<h1>" + Utils::toString(loc.redirect_code)
                + " Redirect</h1><p>→ " + loc.redirect_url + "</p>";
        r.headers["Content-Length"] = Utils::toString(r.body.size());
        r.headers["Content-Type"] = "text/html";
        return (r);
    }

    // 2) Proteger contra directory traversal
    if (rq.uri.find("..") != std::string::npos)
        return forbiddenPageResponse(config);

    // 3) CGI (se extensão combinou)
    std::string ext = getExtension(req.path);
    Logger::log(Logger::INFO, "URI:::: " + rq.uri);
    Logger::log(Logger::INFO, "CGI: " + ext);
    Logger::log(Logger::INFO, "PATH: " + req.path);

    for (std::map<std::string, std::string>::const_iterator it = req.query.begin();
        it != req.query.end(); ++it)
        std::cout << "Query Param: " << it->first << " = " << it->second << std::endl;
    for (size_t i = 0; i < loc.cgi.size(); i++)
    {
        if (ext == loc.cgi[i].extension)
        {
            Logger::log(Logger::INFO, "CGI para extensão: " + ext);
            return CgiHandler::handleCgiRequest(req, config, loc, loc.cgi[i]);
        }
    }

    // 4) Resolver caminho real de arquivo
    std::string root = loc.root.empty() ? config.root : loc.root;
    std::string path = root;

    // Corrigir duplo slash
    if (root[root.size() - 1] == '/' && rq.path[0] == '/')
        path += rq.path.substr(1);
    else if (root[root.size() - 1] != '/' && rq.path[0] != '/')
        path += "/" + rq.path;
    else
        path += rq.path;

    // 5) Métodos HTTP

    // GET
    if (rq.method == "GET")
        return methodGet(config, loc, path, rq.uri);

    // POST
    if (rq.method == "POST")
    {
        if (!loc.upload_dir.empty())
            return methodPostMultipart(rq, loc.upload_dir);
        return methodPost(rq, config, path);
    }

    // DELETE
    if (rq.method == "DELETE")
        return methodDelete(path, config);

    // Método não permitido
    return notAloweMethodResponse(config);
}*/